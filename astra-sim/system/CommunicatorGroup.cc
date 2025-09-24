/******************************************************************************
This source code is licensed under the MIT license found in the
LICENSE file in the root directory of this source tree.
*******************************************************************************/

#include "astra-sim/system/CommunicatorGroup.hh"

#include <algorithm>

#include "astra-sim/system/CollectivePlan.hh"
#include "astra-sim/system/Sys.hh"

using namespace AstraSim;

CommunicatorGroup::CommunicatorGroup(int id,
                                     std::vector<int> involved_NPUs,
                                     Sys* generator) {
    set_id(id);
    this->involved_NPUs = involved_NPUs;
    this->generator = generator;
    std::sort(involved_NPUs.begin(), involved_NPUs.end());
}

CommunicatorGroup::~CommunicatorGroup() {
    for (auto cg : comm_plans) {
        CollectivePlan* cp = cg.second;
        delete cp;
    }
}

void CommunicatorGroup::set_id(int id) {
    assert(id > 0);
    this->id = id;
    this->num_streams = id * 1000000;
}

CollectivePlan* CommunicatorGroup::get_collective_plan(ComType comm_type) {
    if (comm_plans.find(comm_type) != comm_plans.end()) {
        return comm_plans[comm_type];
    }

    if (static_cast<uint64_t>(generator->total_nodes) == involved_NPUs.size()) {
        LogicalTopology* logical_topology =
            generator->get_logical_topology(comm_type);
        std::vector<CollectiveImpl*> collective_implementation =
            generator->get_collective_implementation(comm_type);
        std::vector<bool> dimensions_involved(10, true);
        bool should_be_removed = false;
        comm_plans[comm_type] =
            new CollectivePlan(logical_topology, collective_implementation,
                               dimensions_involved, should_be_removed);
        return comm_plans[comm_type];
    } else {
        // For subset communicator groups, preserve the original collective
        // implementation choices (e.g., direct/halving-doubling) instead of
        // forcing Ring, while constraining the topology membership to the
        // involved NPUs.
        //
        // We construct a 1-D logical topology over only the members of this
        // communicator group, and clone the system-level collective
        // implementation vector so the plan remains consistent with the
        // original system configuration.
        LogicalTopology* logical_topology = new RingTopology(
            RingTopology::Dimension::Local, generator->id, involved_NPUs);

        // Clone implementations so this plan owns its copies.
        std::vector<CollectiveImpl*> impl_clones;
        {
            std::vector<CollectiveImpl*> base_impls =
                generator->get_collective_implementation(comm_type);
            impl_clones.reserve(base_impls.size());
            for (auto* ci : base_impls) {
                impl_clones.push_back((CollectiveImpl*)ci->clone());
            }
        }

        // Mark all (potential) dimensions as involved. The topology itself is
        // 1-D here, so only index 0 will be used, but we keep the vector sized
        // to the implementation list for consistency.
        std::vector<bool> dimensions_involved(impl_clones.size(), true);
        bool should_be_removed = true;
        comm_plans[comm_type] = new CollectivePlan(
            logical_topology, impl_clones, dimensions_involved,
            should_be_removed);
        return comm_plans[comm_type];
    }
    assert(false);
    return nullptr;
}
