#include <cstdint>
#include <utility>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <iostream>
#include <any>
#include <variant>
#include <vector>
#include <string>
#include <map>
#include <ylt/struct_json/json_reader.h>
#include <ylt/struct_json/json_writer.h>
#include <nlohmann/json.hpp>
#include "tmp.h"
//c header
extern "C" {
#include <mvbs/rte/Rte_Dds_Adaptor.h>


#if defined(TARGET_PRODUCT_LINUX) || defined(TARGET_PRODUCT_J6M_ACORE) || defined(TARGET_PRODUCT_MINGW)
#include <mvbs_aux/mvbs_aux.h>
#endif

extern Rte_Dds_Participant Rte_Dds_GetParticipant();
}

using json = nlohmann::json;

typedef websocketpp::server<websocketpp::config::asio> server;
typedef struct reader reader_t;
typedef struct writer writer_t;

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

// pull out the type of messages sent by our config
typedef server::message_ptr message_ptr;



class MVBS {
public:
    
    MVBS() {
        is_initialized = false;
        has_connection = false;
        ptcp = nullptr;
    }
    
   
    // Initialize MVBS system
    bool init() {
        // RegisterData();
        ptcp = Rte_Dds_GetParticipant();
        if (Rte_Dds_Init(ptcp) != RTE_DDS_RETCODE_OK) {
           
            std::cerr << "Rte_Dds_Init failed!" << std::endl;
            return false;
        }
        RegisterDataTmp();
        
        std::cout << "Rte_Dds_Init OK!" << std::endl;
        is_initialized = true;
        return true;
    }
    
    // Set the single connection
    void set_connection(websocketpp::connection_hdl hdl) {
        connection = hdl;
        has_connection = true;
    }
    
    // Clear the connection
    void clear_connection() {
        has_connection = false;
    }
    
    // Check if we have an active connection
    bool is_connected() const {
        return has_connection;
    }

    bool is_init() const {
        return is_initialized;
    }

    // Get the participant
    Rte_Dds_Participant get_participant() const {
        return ptcp;
    }
    
    // Send message to the connection
    bool send_message(const std::string& msg, server* s) {
        if (!is_initialized) {
            std::cerr << "MVBS not initialized" << std::endl;
            return false;
        }
        
        if (!has_connection) {
            std::cerr << "No active connection" << std::endl;
            return false;
        }
        
        try {
            s->send(connection, msg, websocketpp::frame::opcode::text);
            return true;
        } catch (websocketpp::exception const & e) {
            std::cerr << "Failed to send message: " << e.what() << std::endl;
            return false;
        }
    }

    std::pair<FunctionMap::Functions, Rte_Dds_TopicData*> get_data(std::string topic_name,std::string type_name) {
        auto it = data_map.find(topic_name+":"+type_name);
        if (it != data_map.end()) {
            return it->second;
        } else {
            // Dynamically allocate memory for Rte_Dds_TopicData
           
            auto functions = GetDataTmp(type_name);

            Rte_Dds_TopicData* data = new Rte_Dds_TopicData();
            Rte_Dds_PackData(data, functions.getRaw());
            
            data_map[topic_name+":"+type_name] = {functions, data};
            return  data_map[topic_name+":"+type_name];
        }
    }
   

    Rte_Dds_Writer get_writer(std::string topic_name) {
        auto it = writer_map.find(topic_name);
        if (it != writer_map.end()) {
            return it->second;
        }
        
        Rte_Dds_Writer writer = Rte_Dds_GetWriter(ptcp, topic_name.c_str());
        if (writer != nullptr) {
            writer_map[topic_name] = writer;
        }
        return writer;
    }

    Rte_Dds_Reader get_reader(std::string topic_name) {
        auto it = reader_map.find(topic_name);
        if (it != reader_map.end()) {
            return it->second;
        }
        
        Rte_Dds_Reader reader = Rte_Dds_GetReader(ptcp, topic_name.c_str());
        if (reader != nullptr) {
            reader_map[topic_name] = reader;
        }
        return reader;
    }

    std::pair<bool, std::string> write_data(std::string topic_name, std::string type_name, std::string data,Rte_Dds_SequenceNumber *writesn) {
        if (!is_initialized) {
            return {false, "MVBS not initialized"};
        }
        
        if (!has_connection) {
            return {false, "No active connection"};
        }
        
        Rte_Dds_Writer writer = get_writer(topic_name);
        if (writer == nullptr) {
            return {false, "Writer not found"};
        }
        std::cout << "Writing data to topic: " << topic_name << std::endl;
        auto data_pair = get_data(topic_name,type_name);
        //set
        auto& functions = data_pair.first;
        
        functions.set(data,data_pair.second->data);
        Rte_Dds_ReturnType ret = Rte_Dds_TxData(writer, data_pair.second, writesn);
        if (ret != RTE_DDS_RETCODE_OK) {
            return {false, "Failed to write data"};
        }
        return {true, "Data written successfully"};
    }

    websocketpp::connection_hdl get_connection() const {
        return connection;
    }
   
private:
    std::map<std::string, std::pair<FunctionMap::Functions,Rte_Dds_TopicData*>> data_map;
    std::map<std::string, Rte_Dds_Writer> writer_map;
    std::map<std::string, Rte_Dds_Reader> reader_map;
    websocketpp::connection_hdl connection;
    Rte_Dds_Participant ptcp;
    bool is_initialized;
    bool has_connection;
};



MVBS* mvbs_instance = nullptr;
server echo_server;

void Rte_Dds_RxIndication(Rte_Dds_Reader r, Rte_Dds_Instance inst)
{
	(void)inst;

	

	static struct sample_info 	Sample_info;
	const char * topic_name;
	

	topic_name = reader_get_topicname(r);
	
    std::string type_name_str = *reinterpret_cast<std::string*>(Rte_Dds_Reader_Get_Private(r));
    auto data_pair = mvbs_instance->get_data(topic_name,type_name_str);

	
    json notify;
    notify["notify"]="rx_indication";
    notify["topic_name"] = topic_name;
    notify["type_name"] = type_name_str;
    if(mvbs_instance->is_init()){
        if (RTE_DDS_RETCODE_OK == Rte_Dds_RxData(r, true, data_pair.second, &Sample_info)) {
            notify["ok"] = true;
            std::string string = data_pair.first.get(data_pair.second->data);
            notify["data"] = json::parse(string);
            std::string sample_info_str;
            struct_json::to_json(Sample_info, sample_info_str);
            notify["sample_info"] = json::parse(sample_info_str);

        } else {
            notify["ok"] = false;
            notify["message"] = "Failed to read data";
        
        }
        mvbs_instance->send_message(notify.dump(), &echo_server);
    }

}

// Define a callback to handle incoming messages
void on_message(server* s, websocketpp::connection_hdl hdl, message_ptr msg) {
    std::cout << "on_message called with hdl: " << hdl.lock().get()
              << " and message length: " << msg->get_payload().size()
              << std::endl;

    if(mvbs_instance) {
        json reply;
        try {
            
            json obj = json::parse(msg->get_payload());
            
            std::string method = obj["method"].get<std::string>();
            reply["id"] = obj["id"];
            reply["method"] = obj["method"];
            if(method=="write"){
                std::string topic_name = obj["topic_name"].get<std::string>();
                std::string type_name = obj["type_name"].get<std::string>();
                std::string data = obj["data"].dump();
                Rte_Dds_SequenceNumber writesn;
                
              
                auto result = mvbs_instance->write_data(topic_name, type_name, data, &writesn);
                std::string writesn_str;
                struct_json::to_json(writesn, writesn_str);
            
              
                reply["ok"] = result.first;
                reply["message"] = result.second;
                reply["writesn"] = json::parse(writesn_str);
               
                
                
               
            
            } else if(method=="read") {
                std::string topic_name = obj["topic_name"].get<std::string>();
                std::string type_name = obj["type_name"].get<std::string>();
                
                auto data_pair = mvbs_instance->get_data(topic_name,type_name);
                
                Rte_Dds_Reader reader = mvbs_instance->get_reader(topic_name);

                struct sample_info Sample_info;

                Rte_Dds_Reader_Set_Private(reader, reinterpret_cast<void*>(&type_name));
               
                if(Rte_Dds_RxData(reader, true, data_pair.second, &Sample_info)==RTE_DDS_RETCODE_OK){
                    // Convert the data to JSON
                    std::string sample_info_str;
                    struct_json::to_json(Sample_info, sample_info_str);
                    
                    reply["sample_info"] = json::parse(sample_info_str);
                }
                reply["data"] = json::parse(data_pair.first.get(data_pair.second->data));
                reply["ok"] = true;               
            } else if(method=="get_reader_stats"){
                Rte_Dds_TopicStats stats;
                std::string topic_name = obj["topic_name"].get<std::string>();



                Rte_Dds_Reader r = mvbs_instance->get_reader(topic_name);
                if(r){
                    memset(&stats, 0, sizeof(Rte_Dds_TopicStats));
                    if (Rte_Dds_GetReaderStats(r, &stats) != RTE_DDS_RETCODE_OK) {
                        reply["ok"] = false;
                        reply["message"] = "peers match failed";
                    }else{
                        std::string str;
                        struct_json::to_json(stats, str); 
                        reply["ok"] = true;
                        reply["data"]=  json::parse(str);
                        
                    }
                }else{
                    reply["ok"] = false;
                    reply["message"] = "reader not found";
                }

            }
            else if(method=="get_writer_stats"){
                Rte_Dds_TopicStats stats;
                std::string topic_name = obj["topic_name"].get<std::string>();



                Rte_Dds_Writer r = mvbs_instance->get_writer(topic_name);
                if(r){
                    memset(&stats, 0, sizeof(Rte_Dds_TopicStats));
                    if (Rte_Dds_GetWriterStats(r, &stats) != RTE_DDS_RETCODE_OK) {
                        reply["ok"] = false;
                        reply["message"] = "peers match failed";
                    }else{
                        std::string str;
                        struct_json::to_json(stats, str); 
                        std::cout << "get_writer_stats: " << str << std::endl;
                        reply["ok"] = true;
                        reply["data"]=  json::parse(str);
                        
                    }
                }else{
                    reply["ok"] = false;
                    reply["message"] = "reader not found";
                }

            }
            else if(method=="get_writer_peers"){
                std::string topic_name = obj["topic_name"].get<std::string>();
                Rte_Dds_Writer r = mvbs_instance->get_writer(topic_name);
                size_t peer_count = Rte_Dds_GetWriterOnlinePeers(r, nullptr, 0);
                json data = json::array();
                if(peer_count > 0) {
                    std::vector<Rte_Dds_Peers> peers(peer_count);
                    peer_count=Rte_Dds_GetWriterOnlinePeers(r, peers.data(), peer_count);
                    for(size_t i = 0; i < peer_count; ++i) {
                        std::string str;
                        struct_json::to_json(peers[i], str); 
                        data.push_back(json::parse(str));
                    }
                } 

                reply["data"] = data;

            }
            else if(method=="get_reader_peers"){
                std::string topic_name = obj["topic_name"].get<std::string>();
                Rte_Dds_Reader r = mvbs_instance->get_reader(topic_name);
                size_t peer_count = Rte_Dds_GetReaderOnlinePeers(r, nullptr, 0);
                json data = json::array();
                if(peer_count > 0) {
                    std::vector<Rte_Dds_Peers> peers(peer_count);
                    peer_count=Rte_Dds_GetReaderOnlinePeers(r, peers.data(), peer_count);
                    for(size_t i = 0; i < peer_count; ++i) {
                        std::string str;
                        struct_json::to_json(peers[i], str); 
                        data.push_back(json::parse(str));
                    }
                } 

                reply["data"] = data;

            }
            else {
              
                reply["ok"] = false;
                reply["message"] = "unknown_method";
                
                std::string reply_str = reply.dump();
                mvbs_instance->send_message(reply_str, s);
            }
            
            
        }
        catch (const std::exception& e) {
            reply["ok"] = false;
            reply["message"] = e.what();
           
        }
        std::string reply_str = reply.dump();
        mvbs_instance->send_message(reply_str, s);
    }

   
}

// 连接打开时的处理函数
void on_open(server* s, websocketpp::connection_hdl hdl) {
    std::cout << "Connection opened, stopping server from accepting new connections" << std::endl;
    s->stop_listening();   
    mvbs_instance->set_connection(hdl);
}
#define MVBS_APP_LOOP_PERIOD_MS			5

void loop_thread(void* arg) {
    struct mvbs_event_loop* loop = (struct mvbs_event_loop*)arg;
    uint32_t event = 0;
    uint32_t recv_max_pkg = 0;
    while (true) {
       
        event = mvbs_event_loop_wait(loop);
        if(mvbs_instance->is_init() == false) {
            continue;
        }
        if(event&MVBS_EV_RECV){
            Rte_Dds_Ptcp_Recv_Handler(mvbs_instance->get_participant(), recv_max_pkg);
        }
        if (event & MVBS_EV_TIMER) {   
            Rte_Dds_Ptcp_Timer_Handler(mvbs_instance->get_participant());
		}
        
        
    }
}

int main(int argc, char* argv[]) {

    struct mvbs_event_loop		*loop		= NULL;

#ifdef TARGET_PRODUCT_MINGW
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
    setbuf(stdout, NULL);
    mvbs_instance = new MVBS();
	loop = mvbs_event_loop_create(MVBS_APP_LOOP_PERIOD_MS);
    // Initialize thread after loop creation
    std::thread loop_thread_handle(loop_thread, loop);
    loop_thread_handle.detach();
   
    if(mvbs_instance->init()==false){
        exit(-1);
    }
    // Create a server endpoint
   
    int port=20101;
 
    if(argc == 2) {
        port = atoi(argv[1]);
    }

    try {
        // Set logging settings
        echo_server.set_access_channels(websocketpp::log::alevel::all);
        echo_server.clear_access_channels(websocketpp::log::alevel::frame_payload);

        // Initialize Asio
        echo_server.init_asio();

        // Register our message handler
        echo_server.set_message_handler(bind(&on_message,&echo_server,::_1,::_2));
        
        // Register our open handler to stop listening after first connection
        echo_server.set_open_handler(bind(&on_open,&echo_server,::_1));

        // Listen on port 9002
        echo_server.listen(port);

        // Start the server accept loop
        echo_server.start_accept();

        // Start the ASIO io_service run loop
        echo_server.run();
    } catch (websocketpp::exception const & e) {
        std::cout << e.what() << std::endl;
    } catch (...) {
        std::cout << "other exception" << std::endl;
    }
}
