/*! \file mesh.cxx
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

#include "height-field.hpp"
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

    this->vBuf = new cs237::VertexBuffer(app, grp.nVerts * sizeof(Vertex));
    this->vBufMem = new cs237::MemoryObj(app, this->vBuf->requirements());
    this->vBuf->bindMemory(this->vBufMem);

    this->iBuf = new cs237::IndexBuffer(app, grp.nIndices, grp.nIndices * sizeof(uint32_t));
    this->iBufMem = new cs237::MemoryObj(app, this->iBuf->requirements());
    this->iBuf->bindMemory(this->iBufMem);

    // vertex buffer initialization; first convert struct of arrays to array of structs
    std::vector<Vertex> verts(grp.nVerts);
    for (int i = 0;  i < grp.nVerts;  ++i) {
        verts[i].pos = grp.verts[i];
        verts[i].norm = grp.norms[i];
        verts[i].txtCoord = grp.txtCoords[i];
    }
    // copy data
    this->vBufMem->copyTo(verts.data());

    // index buffer initialization
    this->iBufMem->copyTo(grp.indices);

    /** HINT: other initialization, such as color and normal maps */
}

Mesh::Mesh (cs237::Application *app, HeightField *hf)
  : vBuf(nullptr), vBufMem(nullptr), iBuf(nullptr), iBufMem(nullptr),
    prim(0 /** HINT: your choice */), nIndices(grp.nIndices)
{
}

Mesh::~Mesh ()
{
    /** HINT: delete Vulkan resources here */
}

void Mesh::draw (VkCommandBuffer cmdBuf)
{
    /** HINT: record index-mode drawing commands here */
}
