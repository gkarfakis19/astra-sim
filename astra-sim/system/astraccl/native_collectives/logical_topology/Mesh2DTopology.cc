/******************************************************************************
This source code is licensed under the MIT license found in the
LICENSE file in the root directory of this source tree.
*******************************************************************************/

#include "astra-sim/system/astraccl/native_collectives/logical_topology/Mesh2DTopology.hh"
#include "astra-sim/common/Logging.hh"

#include <cassert>
#include <iostream>

using namespace std;
using namespace AstraSim;

Mesh2DTopology::Mesh2DTopology(Dimension dimension, int id, std::vector<int> NPUs)
    : BasicLogicalTopology(BasicLogicalTopology::BasicTopology::Mesh2D) {
    name = "local";
    if (dimension == Dimension::Vertical) {
        name = "vertical";
    } else if (dimension == Dimension::Horizontal) {
        name = "horizontal";
    }
    this->id = id;
    this->total_nodes_in_mesh = NPUs.size();
    this->dimension = dimension;
    this->offset = -1;
    this->index_in_mesh = -1;
    
    // Initialize dims
    if (dimension == Dimension::Local) {
        dims = {total_nodes_in_mesh};
    } else if (dimension == Dimension::Horizontal || dimension == Dimension::Vertical) {
        int dim_size = static_cast<int>(std::sqrt(total_nodes_in_mesh));
        dims = {dim_size, dim_size};
    } else {
        dims = {total_nodes_in_mesh};
    }

    for (int i = 0; i < total_nodes_in_mesh; i++) {
        id_to_index[NPUs[i]] = i;
        index_to_id[i] = NPUs[i];
        if (id == NPUs[i]) {
            index_in_mesh = i;
        }
    }

    LoggerFactory::get_logger("system::topology::Mesh2DTopology")
        ->info("custom mesh, id: {}, dimension: {} total nodes in mesh: {} "
               "index in mesh: {} total nodes in mesh {}",
               id, name, total_nodes_in_mesh, index_in_mesh,
               total_nodes_in_mesh);

    assert(index_in_mesh >= 0);
}

Mesh2DTopology::Mesh2DTopology(Dimension dimension,
                           int id,
                           int total_nodes_in_mesh,
                           int index_in_mesh,
                           int offset)
    : BasicLogicalTopology(BasicLogicalTopology::BasicTopology::Mesh2D) {
    name = "local";
    if (dimension == Dimension::Vertical) {
        name = "vertical";
    } else if (dimension == Dimension::Horizontal) {
        name = "horizontal";
    }
    if (id == 0) {
        LoggerFactory::get_logger("system::topology::Mesh2DTopology")
            ->info("mesh of node 0, id: {} dimension: {} total nodes in mesh: "
                   "{} index in mesh: {} offset: {} total nodes in mesh: {}",
                   id, name, total_nodes_in_mesh, index_in_mesh, offset,
                   total_nodes_in_mesh);
    }
    this->id = id;
    this->total_nodes_in_mesh = total_nodes_in_mesh;
    this->index_in_mesh = index_in_mesh;
    this->dimension = dimension;
    this->offset = offset;

    // Initialize dims
    if (dimension == Dimension::Local) {
        dims = {total_nodes_in_mesh};
    } else if (dimension == Dimension::Horizontal || dimension == Dimension::Vertical) {
        int dim_size = static_cast<int>(std::sqrt(total_nodes_in_mesh));
        dims = {dim_size, dim_size};
    } else {
        dims = {total_nodes_in_mesh};
    }

    id_to_index[id] = index_in_mesh;
    index_to_id[index_in_mesh] = id;
    int tmp = id;
    for (int i = 0; i < total_nodes_in_mesh - 1; i++) {
        tmp = get_receiver_homogeneous(tmp, Mesh2DTopology::Direction::Clockwise,
                                       offset);
    }
}


int Mesh2DTopology::get_receiver_homogeneous(int node_id,
                                           Direction direction,
                                           int offset) {
    assert(id_to_index.find(node_id) != id_to_index.end());
    int index = id_to_index[node_id];
    if (direction == Mesh2DTopology::Direction::Clockwise) {
        int receiver = node_id + offset;
        if (index == total_nodes_in_mesh - 1) {
            receiver -= (total_nodes_in_mesh * offset);
            index = 0;
        } else {
            index++;
        }
        if (receiver < 0) {
            LoggerFactory::get_logger("system::topology::Mesh2DTopology")
                ->critical("at dim: {} at id: {} dimension: {} index: {}, node "
                           "id: {}, offset: {}, index_in_mesh {} receiver {}",
                           name, id, name, index, node_id, offset,
                           index_in_mesh, receiver);
        }
        assert(receiver >= 0);
        id_to_index[receiver] = index;
        index_to_id[index] = receiver;
        return receiver;
    } else {
        int receiver = node_id - offset;
        if (index == 0) {
            receiver += (total_nodes_in_mesh * offset);
            index = total_nodes_in_mesh - 1;
        } else {
            index--;
        }
        if (receiver < 0) {
            LoggerFactory::get_logger("system::topology::Mesh2DTopology")
                ->critical("at dim: {} at id: {} dimension: {} index: {}, node "
                           "id: {}, offset: {}, index_in_mesh {} receiver {}",
                           name, id, name, index, node_id, offset,
                           index_in_mesh, receiver);
        }
        assert(receiver >= 0);
        id_to_index[receiver] = index;
        index_to_id[index] = receiver;
        return receiver;
    }
}



std::vector<int> Mesh2DTopology::get_receivers(int node_id, Mesh2DTopology::Direction direction) const {
    assert(id_to_index.find(node_id) != id_to_index.end());
    int index = id_to_index.at(node_id);
    std::vector<int> receivers;

    // Handle 1D mesh (line)
    if (dims.empty() || dims.size() == 1) {
        int n = total_nodes_in_mesh;
        if (n <= 1) return receivers;

        if (direction == Mesh2DTopology::Direction::Clockwise) {
            if (index + 1 < n)
                receivers.push_back(index_to_id.at(index + 1));
        } else {
            if (index - 1 >= 0)
                receivers.push_back(index_to_id.at(index - 1));
        }
        return receivers;
    }

    // 2D mesh (no wrap-around)
    const int rows = dims[0];
    const int cols = dims[1];
    int row = index / cols;
    int col = index % cols;

    if (direction == Mesh2DTopology::Direction::Clockwise) {
        // Right neighbor
        if (col + 1 < cols)
            receivers.push_back(index_to_id.at(index + 1));
        // Down neighbor
        if (row + 1 < rows)
            receivers.push_back(index_to_id.at(index + cols));
    } else { // Anticlockwise
        // Left neighbor
        if (col - 1 >= 0)
            receivers.push_back(index_to_id.at(index - 1));
        // Up neighbor
        if (row - 1 >= 0)
            receivers.push_back(index_to_id.at(index - cols));
    }

    return receivers;
}



std::vector<int> Mesh2DTopology::get_senders(int node_id, Mesh2DTopology::Direction direction) const {
    assert(id_to_index.find(node_id) != id_to_index.end());
    int index = id_to_index.at(node_id);
    std::vector<int> senders;

    // Handle 1D mesh (line)
    if (dims.empty() || dims.size() == 1) {
        int n = total_nodes_in_mesh;
        if (n <= 1) return senders;

        if (direction == Mesh2DTopology::Direction::Anticlockwise) {
            if (index + 1 < n)
                senders.push_back(index_to_id.at(index + 1));
        } else {
            if (index - 1 >= 0)
                senders.push_back(index_to_id.at(index - 1));
        }
        return senders;
    }

    // 2D mesh (no wrap-around)
    const int rows = dims[0];
    const int cols = dims[1];
    int row = index / cols;
    int col = index % cols;

    if (direction == Mesh2DTopology::Direction::Anticlockwise) {
        // Right neighbor (sender from right)
        if (col + 1 < cols)
            senders.push_back(index_to_id.at(index + 1));
        // Down neighbor (sender from below)
        if (row + 1 < rows)
            senders.push_back(index_to_id.at(index + cols));
    } else { // Clockwise
        // Left neighbor (sender from left)
        if (col - 1 >= 0)
            senders.push_back(index_to_id.at(index - 1));
        // Up neighbor (sender from above)
        if (row - 1 >= 0)
            senders.push_back(index_to_id.at(index - cols));
    }

    return senders;
}



int Mesh2DTopology::get_index_in_mesh() {
    return index_in_mesh;
}

Mesh2DTopology::Dimension Mesh2DTopology::get_dimension() {
    return dimension;
}

int Mesh2DTopology::get_num_of_nodes_in_dimension(int dimension) {
    return get_nodes_in_mesh();
}

int Mesh2DTopology::get_nodes_in_mesh() {
    return total_nodes_in_mesh;
}

bool Mesh2DTopology::is_enabled() {
    assert(offset > 0);
    int tmp_index = index_in_mesh;
    int tmp_id = id;
    while (tmp_index > 0) {
        tmp_index--;
        tmp_id -= offset;
    }
    if (tmp_id == 0) {
        return true;
    }
    return false;
}
