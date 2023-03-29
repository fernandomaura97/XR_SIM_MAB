#include <assert.h> 
 #include<vector> 
template <class T> class compcxx_array { public: 
virtual ~compcxx_array() { for (typename std::vector<T*>::iterator i=m_elements.begin();i!=m_elements.end();i++) delete (*i); } 
void SetSize(unsigned int n) { for(unsigned int i=0;i<n;i++)m_elements.push_back(new T); } 
T& operator [] (unsigned int i) { assert(i<m_elements.size()); return(*m_elements[i]); } 
unsigned int size() { return m_elements.size();} 
private: std::vector<T*> m_elements; }; 
class compcxx_component; 
template <class T> class compcxx_functor {public: 
void Connect(compcxx_component&_c, T _f){ c.push_back(&_c); f.push_back(_f); } 
protected: std::vector<compcxx_component*>c; std::vector<T> f; }; 
class compcxx_component { public: 
typedef void  (compcxx_component::*AccessPoint_out_to_wireless_f_t)(data_packet &packet);
typedef void  (compcxx_component::*AccessPoint_out_to_network_f_t)(data_packet &packet);
typedef void  (compcxx_component::*AccessPoint_out_packet_f_t)(data_packet &frame);
typedef void  (compcxx_component::*Network_out_to_apps_f_t)(data_packet &packet);
typedef void  (compcxx_component::*Network_out_to_APs_f_t)(data_packet &packet);
typedef void  (compcxx_component::*Station_out_to_wireless_f_t)(data_packet &packet);
typedef void  (compcxx_component::*Station_out_to_app_f_t)(data_packet &packet);
typedef void  (compcxx_component::*Station_out_packet_f_t)(data_packet &frame);
typedef void  (compcxx_component::*TrafficGeneratorApp_out_f_t)(data_packet &packet);
typedef void  (compcxx_component::*XRClient_out_f_t)(data_packet &packet);
typedef void  (compcxx_component::*XRServer_out_f_t)(data_packet &packet);
typedef void  (compcxx_component::*CSMACAChannel1_out_slot_f_t)(SLOT_indicator &slot);
};
