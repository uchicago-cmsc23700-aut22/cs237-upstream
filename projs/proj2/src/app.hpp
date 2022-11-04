/*! \file app.hpp
 *
 * CS23700 Autumn 2022 Sample Code for Project 2
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2022 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _APP_HPP_
#define _APP_HPP_

#include "cs237.hpp"
#include "scene.hpp"

//! The Project 1 Application class
class Proj2 : public cs237::Application {
public:
    Proj2 (std::vector<const char *> &args);
    ~Proj2 ();

    //! run the application
    void run () override;

    //! access function for the scene
    Scene *scene () { return &this->_scene; }

protected:
    Scene _scene;               //!< the scene to be rendered

};

#endif // !_APP_HPP_

