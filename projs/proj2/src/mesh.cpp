/*! \file mesh.cxx
 *
 * CS23700 Autumn 2022 Sample Code for Project 2
 *
 * \author John Reppy
 */

/* CMSC23700 Project 1 sample code (Autumn 2022)
 *
 * COPYRIGHT (c) 2022 John Reppy (http://www.cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "vertex.hpp"
#include "mesh.hpp"
#include <vector>

Mesh::Mesh (cs237::Application *app, VkPrimitiveTopology p, OBJ::Group const &grp)
  : vBuf(nullptr), vBufMem(nullptr), iBuf(nullptr), iBufMem(nullptr),
    prim(p), nIndices(grp.nIndices)
{
    if (grp.norms == nullptr) {
        ERROR("missing normals in model mesh");
    }
    if (grp.txtCoords == nullptr) {
         ERROR("missing texture coordinates in model mesh");
    }

    /** HINT: initialize the buffers etc here; you will have to convert from
     ** the SOA representation in the group to a AOS representation to initialize
     ** the vertex buffer.
     */

}

Mesh::~Mesh ()
{
    /** HINT: delete Vulkan resources here */
}

void Mesh::draw (VkCommandBuffer cmdBuf)
{
    /** HINT: record index-mode drawing commands here */
}
