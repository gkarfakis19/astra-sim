/******************************************************************************
This source code is licensed under the MIT license found in the
LICENSE file in the root directory of this source tree.
*******************************************************************************/

#ifndef __MESH2D_HH__
#define __MESH2D_HH__

#include "astra-sim/system/MemBus.hh"
#include "astra-sim/system/MyPacket.hh"
#include "astra-sim/system/astraccl/Algorithm.hh"
#include "astra-sim/system/astraccl/native_collectives/logical_topology/Mesh2DTopology.hh"

namespace AstraSim {

class Mesh2D : public Algorithm {
  public:
    Mesh2D(ComType type,
         int id,
         Mesh2DTopology* mesh_topology,
         uint64_t data_size,
         Mesh2DTopology::Direction direction,
         InjectionPolicy injection_policy);
    virtual void run(EventType event, CallData* data);
    void process_stream_count();
    void release_packets();
    virtual void process_max_count();
    void reduce();
    bool iteratable();
    virtual int get_non_zero_latency_packets();
    void insert_packet(Callable* sender);
    bool ready();
    void exit();

    Mesh2DTopology::Direction dimension;
    Mesh2DTopology::Direction direction;
    MemBus::Transmition transmition;
    int zero_latency_packets;
    int non_zero_latency_packets;
    int id;
    std::vector<int> curr_receivers;
    std::vector<int> curr_senders;
    int nodes_in_dim;
    int nodes_in_mesh;
    int stream_count;
    int max_count;
    int remained_packets_per_max_count;
    int remained_packets_per_message;
    int parallel_reduce;
    InjectionPolicy injection_policy;
    std::list<MyPacket> packets;
    bool toggle;
    long free_packets;
    long total_packets_sent;
    long total_packets_received;
    uint64_t msg_size;
    std::list<MyPacket*> locked_packets;
    bool processed;
    bool send_back;
    bool NPU_to_MA;
    bool m_bidirectional;
};

}  // namespace AstraSim

#endif /* __MESH2D_HH__ */
