/*
 *  Copyright 2020, Sebastian Pütz
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *
 *  3. Neither the name of the copyright holder nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 *  authors:
 *    Sebastian Pütz <spuetz@uni-osnabrueck.de>
 *
 */

#include "mesh_layers/height_diff_layer.h"

#include <lvr2/algorithm/GeometryAlgorithms.hpp>
#include <lvr2/algorithm/NormalAlgorithms.hpp>
#include <pluginlib/class_list_macros.hpp>

PLUGINLIB_EXPORT_CLASS(mesh_layers::HeightDiffLayer, mesh_map::AbstractLayer)

namespace mesh_layers
{
bool HeightDiffLayer::readLayer()
{
  RCLCPP_INFO_STREAM(node_->get_logger(), "Try to read height differences from map file...");
  auto height_diff_opt = mesh_io_ptr->getDenseAttributeMap<lvr2::DenseVertexMap<float>>("height_diff");

  if (height_diff_opt)
  {
    RCLCPP_INFO_STREAM(node_->get_logger(), "Height differences have been read successfully.");
    height_diff = height_diff_opt.get();

    return computeLethals();
  }

  return false;
}

bool HeightDiffLayer::computeLethals()
{
  RCLCPP_INFO_STREAM(node_->get_logger(), "Compute lethals for \"" << layer_name << "\" (Height Differences Layer) with threshold "
                                           << config.threshold);
  lethal_vertices.clear();
  for (auto vH : height_diff)
  {
    if (height_diff[vH] > config.threshold)
      lethal_vertices.insert(vH);
  }
  RCLCPP_INFO_STREAM(node_->get_logger(), "Found " << lethal_vertices.size() << " lethal vertices.");
  return true;
}

bool HeightDiffLayer::writeLayer()
{
  RCLCPP_INFO_STREAM(node_->get_logger(), "Saving height_differences to map file...");
  if (mesh_io_ptr->addDenseAttributeMap(height_diff, "height_diff"))
  {
    RCLCPP_INFO_STREAM(node_->get_logger(), "Saved height differences to map file.");
    return true;
  }
  else
  {
    RCLCPP_ERROR_STREAM(node_->get_logger(), "Could not save height differences to map file!");
    return false;
  }
}

float HeightDiffLayer::threshold()
{
  return config.threshold;
}

bool HeightDiffLayer::computeLayer()
{
  height_diff = lvr2::calcVertexHeightDifferences(*mesh_ptr, config.radius);
  return computeLethals();
}

lvr2::VertexMap<float>& HeightDiffLayer::costs()
{
  return height_diff;
}

void HeightDiffLayer::reconfigureCallback(mesh_layers::HeightDiffLayerConfig& cfg, uint32_t level)
{
  bool notify = false;
  RCLCPP_INFO_STREAM(node_->get_logger(), "New height diff layer config through dynamic reconfigure.");

  if (first_config)
  {
    config = cfg;
    first_config = false;
    return;
  }

  if (config.threshold != cfg.threshold)
  {
    computeLethals();
    notify = true;
  }

  config = cfg;
  if (notify)
    notifyChange();
}

bool HeightDiffLayer::initialize(const std::string& name)
{
  first_config = true;
  reconfigure_server_ptr = boost::shared_ptr<dynamic_reconfigure::Server<mesh_layers::HeightDiffLayerConfig>>(
      new dynamic_reconfigure::Server<mesh_layers::HeightDiffLayerConfig>(private_nh));

  config_callback = boost::bind(&HeightDiffLayer::reconfigureCallback, this, _1, _2);
  reconfigure_server_ptr->setCallback(config_callback);
  return true;
}

} /* namespace mesh_layers */
