/*! \file instance.hpp
 *
 * CS23700 Autumn 2022 Sample Code for Project 2
 *
 * \author John Reppy
 */

/* CMSC23700 Project 2 sample code (Autumn 2022)
 *
 * COPYRIGHT (c) 2022 John Reppy (http://www.cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _INSTANCE_HPP_
#define _INSTANCE_HPP_

#include "cs237.hpp"
#include "mesh.hpp"
#include <memory>

//! An instance of a graphical object in the scene
struct Instance {
    std::shared_ptr<Mesh> mesh; //!< the mesh representing the object.  Note that
                                //!< we use a shared pointer so that the mesh only
                                //!< gets freed once when we are deallocating the
                                //!< instances in the scene.
    glm::vec3 pos;              //!< the position of the object in world space
    glm::vec3 color;            //!< the wireframe/flat color of the object
    glm::mat4 toWorld;          //!< affine transform from object space to world space
    glm::mat3 normToWorld;      //!< linear transform that maps object-space normals
                                //!  to world-space normals
/** HINT: you may want to add additional fields to track the Vulkan state associated
 ** with the instance.
 **/
};

#endif /*! _INSTANCE_HPP_ */
