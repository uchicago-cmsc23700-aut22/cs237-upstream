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

    // compute tangent vectors; we do this by summing the tangent and bitangent
    // vectors for each occurrence of a vertex in a triangle and then normalizing
    // the result.
    std::vector<glm::vec3> tan(grp.nVerts);
    std::vector<glm::vec3> bitan(grp.nVerts);
    uint32_t nTris = grp.nIndices / 3;
    assert (nTris * 3 == grp.nIndices);
    for (int tri = 0;  tri < nTris;  tri++) {
        // get the indices for the triangle
        uint32_t i1 = grp.indices[3*tri + 0];
        uint32_t i2 = grp.indices[3*tri + 1];
        uint32_t i3 = grp.indices[3*tri + 2];
        // get the vertices for the triangle
        glm::vec3 v1 = grp.verts[i1];
        glm::vec3 v2 = grp.verts[i2];
        glm::vec3 v3 = grp.verts[i3];
        // get the texture coordinates for the triangle
        glm::vec2 vt1 = grp.txtCoords[i1];
        glm::vec2 vt2 = grp.txtCoords[i2];
        glm::vec2 vt3 = grp.txtCoords[i3];
        // the sides of the triangle as a 3x2 matrix
        glm::mat3x2 Q = glm::mat3x2(
            v2.x - v1.x, v3.x - v1.x,   // column one
            v2.y - v1.y, v3.y - v2.y,   // column two
            v2.z - v1.z, v3.z - v1.z);  // column three
        // the sides in tangent space as a 2x2 matrix
        glm::mat2x2 ST = glm::mat2x2 (
            vt2.x - vt1.x, vt3.x - vt1.x,       // first column
            vt2.y - vt1.y, vt3.y - vt1.y);      // second column
        // Q = ST * [T B]^T, so multiply Q by ST^{-1}
        glm::mat3x2 TB = glm::inverse(ST) * Q;
        // extract rows T and B
        glm::vec3 t = glm::vec3(TB[0][0], TB[1][0], TB[2][0]);
        glm::vec3 b = glm::vec3(TB[0][1], TB[1][1], TB[2][1]);
        // add to vector sums
        tan[i1] += t;
        tan[i2] += t;
        tan[i3] += t;
        bitan[i1] += b;
        bitan[i2] += b;
        bitan[i3] += b;
    }
    // compute extended tangents for vertices
    for (int i = 0;  i < grp.nVerts;  i++) {
        glm::vec3 n = grp.norms[i];
        glm::vec3 t = glm::normalize(tan[i]);
      // orthogonalize
        t = glm::normalize(t - n * dot(n, t));
        float w = (glm::dot(glm::cross(n, t), bitan[i]) < 0.0f ? -1.0f : 1.0f);
        verts[i].tan = glm::vec4(t, w);
    }

    // copy data
    this->vBufMem->copyTo(verts.data());

    // index buffer initialization
    this->iBufMem->copyTo(grp.indices);

    /** HINT: other initialization, such as color and normal maps */
}

Mesh::Mesh (cs237::Application *app, HeightField *hf)
  : vBuf(nullptr), vBufMem(nullptr), iBuf(nullptr), iBufMem(nullptr),
    prim(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
{
    // allocate the vertex buffer
    this->vBuf = new cs237::VertexBuffer(app, hf->numVerts() * sizeof(Vertex));
    this->vBufMem = new cs237::MemoryObj(app, this->vBuf->requirements());
    this->vBuf->bindMemory(this->vBufMem);

    /** HINT: you will need to compute the Vertex values for the grid points in
     ** the heightfield and then initialize the vertex buffer.
     */

    /** HINT: you will need to create and intialize the index buffer; the indices
     ** will depend on the topology that you choose.  Remember to initialize the
     ** nIndices field.
     **/

    /** HINT: other initialization, such as color and normal maps */
}

Mesh::~Mesh ()
{
    delete this->cMap;
    delete this->nMap;
    delete this->vBufMem;
    delete this->vBuf;
    delete this->iBufMem;
    delete this->iBuf;

}

void Mesh::draw (VkCommandBuffer cmdBuf)
{
    /** HINT: record index-mode drawing commands here */
}
