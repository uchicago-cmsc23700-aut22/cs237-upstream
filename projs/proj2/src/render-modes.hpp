/*! \file render-modes.hpp
 *
 * Constants that specify the different render modes.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2022 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _RENDER_MODES_HPP_
#define _RENDER_MODES_HPP_

//! the different rendering modes
constexpr int kWireframe = 0;           //!< wireframe mode
constexpr int kFlat = 1;                //!< flat shading mode
constexpr int kDiffuse = 2;             //!< diffuse lighting
constexpr int kTextured = 3;            //!< textured shading
constexpr int kNormal = 4;              //!< use normal map shading
#ifdef PROJ2_EXTRA_CREDIT
constexpr int kExtreme = 5;             //!< use both textured and normal-map shading
constexpr int kNumModes = 6;
#else
constexpr int kNumModes = 5;
#endif

#endif //! _RENDER_MODES_HPP_
