#pragma once

#include <string>
#include <vector>

#include "drake/multibody/joints/floating_base_types.h"
#include "drake/multibody/parsers/model_instance_id_table.h"
#include "drake/multibody/parsers/package_map.h"
#include "drake/multibody/rigid_body_frame.h"
#include "drake/multibody/rigid_body_tree.h"

namespace drake {
namespace parsers {

/// Resolves the fully-qualified name of a file. If @p filename starts with
/// "package:", the ROS packages specified in @p package_map are searched.
/// Otherwise, @p filename is appended to the end of @p root_dir and checked
/// for existence. If the file does not exist or is not found, a warning is
/// printed to `std::cerr` and an empty string is returned.
///
/// @param[in] filename The name of the file to find.
///
/// @param[in] package_map A map where the keys are ROS package names and the
/// values are the paths to the packages. This is only used if @p filename
/// starts with "package:".
///
/// @param[in] root_dir The root directory to look in. This is only used when
/// @p filename does not start with "package:".
///
/// @return The file's fully-qualified name or an empty string if the file is
/// not found or does not exist.
std::string ResolveFilename(const std::string& filename,
                            const PackageMap& package_map,
                            const std::string& root_dir);

// TODO(liang.fok): Deprecate this method. See: #3361.
/**
 * Adds a floating joint to each body specified by @p body_indices that does
 * not already have a parent.
 *
 * This method is only intended to be called by parsers since parsers add bodies
 * to the RigidBodyTree _en masse_. The logic in this method is necessary to
 * identify which of the rigid bodies specified by @p body_indices get floating
 * joints.
 *
 * When manually adding a model instance to the RigidBodyTree, i.e., directly
 * using the C++ API rather than via a parser, this method should _not_ be
 * necessary since floating joints can be directly added by calling
 * RigidBody::setJoint().
 *
 * @param[in] floating_base_type The floating joint's type.
 *
 * @param[in] body_indices A list of body indexes to check. A floating joint is
 * added to any body in this list that does not have a parent joint.
 *
 * @param[in] weld_to_frame The frame to which the floating joint should attach
 * the parent-less non-world bodies. This parameter may be nullptr, in which
 * case the body is welded to the world with zero offset.
 *
 * @param[in] pose_map A mapping where the key is the body's name and the value
 * is the transform from the frame of the body to the frame of the model to
 * which the body belongs. This parameter will may be nullptr, in which case an
 * identity transform is used.
 *
 * @param[out] tree The RigidBodyTree to which to add the floating joints.
 *
 * @return The number of floating joint added to this rigid body tree.
 *
 * @throws A std::runtime_error if the floating_base_type is unrecognized or
 * zero floating joints were added to the model.
 */
int AddFloatingJoint(
    multibody::joints::FloatingBaseType floating_base_type,
    const std::vector<int>& body_indices,
    const std::shared_ptr<RigidBodyFrame<double>> weld_to_frame,
    const PoseMap* pose_map,
    RigidBodyTree<double>* tree);

}  // namespace parsers
}  // namespace drake
