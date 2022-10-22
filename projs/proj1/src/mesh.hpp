/*! \file mesh.hpp
 *
 * CS23700 Autumn 2022 Sample Code for Project 1
 *
 * \author John Reppy
 */

/* CMSC23700 Project 1 sample code (Autumn 2022)
 *
 * COPYRIGHT (c) 2022 John Reppy (http://www.cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _MESH_HPP_
#define _MESH_HPP_

#include "cs237.hpp"
#include "obj.hpp"

//! the information needed to render a mesh
struct Mesh {
    VkDevice device;            //!< the Vulkan device
    cs237::VertexBuffer *vBuf;  //!< vertex-array for this mesh
    cs237::MemoryObj *vBufMem;  //!< device memory for `vBuf`
    cs237::IndexBuffer *iBuf;   //!< the index array
    cs237::MemoryObj *iBufMem;  //!< device memory for `idxBuf`
    VkPrimitiveTopology prim;   //!< the primitive type for rendering the mesh
                                //!  (e.g., VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
    int nIndices;               //!< the number of vertex indices
    cs237::Texture2D *tex;      //!< the texture for the object

    //! create a Mesh object by allocating buffers for it.  The buffer data is
    //! loaded separately.
    //! \param app  the owning app
    //! \param p    the topology of the vertices; for Project 1, it should
    //!             be VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
    //! \param grp  a `Group` from an object that this mesh defines
    Mesh (cs237::Application *app, VkPrimitiveTopology p, OBJ::Group const &grp);

    //! Mesh destuctor
    ~Mesh ();

    //! record commands in the command buffer to draw the mesh using
    //! `vkCmdDrawIndexed`.
    void draw (VkCommandBuffer cmdBuf);

};

#endif // !_MESH_HPP_
