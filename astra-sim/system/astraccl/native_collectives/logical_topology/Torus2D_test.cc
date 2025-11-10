/******************************************************************************
This source code is licensed under the MIT license found in the
LICENSE file in the root directory of this source tree.
*******************************************************************************/

#include "astra-sim/system/astraccl/native_collectives/logical_topology/Torus2D_test.hh"

using namespace std;
using namespace AstraSim;

Torus2D_test::Torus2D_test(int id, int total_nodes, int local_dim) {
    // Compute horizontal dimension size
    int horizontal_dim = total_nodes / local_dim;

    // Each dimension is a ring topology
    vertical_dimension = new RingTopology(RingTopology::Dimension::Horizontal, id,
                                       local_dim, id % local_dim, 1);

    horizontal_dimension = new RingTopology(
        RingTopology::Dimension::Vertical, id, horizontal_dim,
        id / local_dim, local_dim);
}

Torus2D_test::~Torus2D_test() {
    delete horizontal_dimension;
    delete vertical_dimension;
}

int Torus2D_test::get_num_of_dimensions() {
    return 2;
}

int Torus2D_test::get_num_of_nodes_in_dimension(int dimension) {
    if (dimension == 0) {
        return horizontal_dimension->get_num_of_nodes_in_dimension(0);
    } else if (dimension == 1) {
        return vertical_dimension->get_num_of_nodes_in_dimension(0);
    }
    return -1;
}

BasicLogicalTopology* Torus2D_test::get_basic_topology_at_dimension(int dimension,
                                                               ComType type) {
    if (dimension == 0) {
        return horizontal_dimension;
    } else if (dimension == 1) {
        return vertical_dimension;
    }
    return NULL;
}
