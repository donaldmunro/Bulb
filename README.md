# Bulb
A C++ scenegraph for [filament](https://github.com/google/filament).

![Screenshot](screenshot.png?raw=true "Orbits sample screenshot")

Bulb is a simple scenegraph for filament which builds a filament Scene from scene graph nodes.

Currently available scene graph nodes include:

* Node - The abstract base class.
* Composite - A Node that can have children.
* Transform - Base class for transforms.
* AffineTransform - Transforms consisting of a rotation, translation and scaling. The order in
which the transformations are applied can be specified. Rotations may be specified as matrices
or quaternions.
* CustomTransform - A transform node specified by a 4x4 matrix supplied by the user.
* Drawable - Base class for leaf nodes that can be rendered. Drawable contains the root Entity
to be rendered. It also contains an optional internal Transform which can be used to help reduce tree
depth by not requiring an extra node for the final transform before the leaf Drawable.
* Geometry - A simple Drawable having a single rendered entity (held in Drawable) and a single
Vertex-and-IndexBuffer that can optionally have a Material or inherit a Material from a predecessor
Material node. Can also open a filament filamesh format file.
* MultiGeometry - A multi-Entity Drawable with the root entity held in Drawable and the remaining
children entities in MultiGeometry. Can load a .gltf 3D format model using filament gltfio, (which
does not support multithreading at the moment so loading models is a bottleneck).
* Material - A Composite whose descendents can inherit the material specified in the node.
* PositionalLight - A light that is positioned using predecessor transform nodes.

Nodes still to be developed:

* SwitchNode - To allow selection of a node to be displayed based on user specified criteria.
* LODNode - Level of Detail node.
* Other to be ascertained.

The *visitor* pattern may be used to process nodes, with visitors specified in nodes/Visitor.hh.
Currently there is only one visitor specified, viz *RenderVisitor*, which builds the scene.

Transform derived nodes may be animated. This is done by first setting a custom animation parameter
(a void *) for the Transform. The Transform animate method animates the
transform by invoking a C++ callable with arguments containing the transform (this) and the animation parameters (see samples/src/orbitanim.cc/h).

The SceneGraph class provides a *Facade* for the scenegraph. It supplies methods for creating nodes,
adopting existing nodes, rendering, accessing animatable transform nodes and finding nodes by name.

In order to retain Android compatibility most file access is done via the AssetReader class which
uses a #ifdef to switch between reading contents of Android assets or desktop files. The
MultiGeometry gltf reader may require access to /sdcard/Documents as it copies the gltf directory to a
local directory (or unzips a gltf zip to a local directory).

## Building

Building requires a filament source directory in a known location specified in the FILAMENT_DIR
CMake variable in CMakeLists.txt. To build filament (assuming location /opt/filament):

~~~~
cd /opt/filament
./build.sh -j -p desktop debug
./build.sh -j -p desktop -i debug
~~~~
For Android:
~~~~
./build.sh -v  -j -p android debug
./build.sh -v  -j -p android -i debug
~~~~

Loading gltf files from zip archives requires libzip to be findable by CMake (not required).
The samples are best used with SDL2 although they should fall back to barebones XLib or Win32.
libpng is required for screenshots in the orbits sample.

##Building for Android 
Bulb requires the full gltfio for loading gltf models but the Android Filament build does not currently
link the full libgltfio.a library. To get around this alter the CMakeLists.txt file in 
${FILAMENT-DIR}/libs/gltfio/ to remove ANDROID from the if statement below:

~~~~
#if (NOT WEBGL AND NOT ANDROID AND NOT IOS)
if (NOT WEBGL AND NOT IOS)

    # ==================================================================================================
    # Link the core library against filamat to create the "full" library
    # ==================================================================================================
~~~~

Also remove bluegl and imageio from the ${FILAMENT_BASELIBS} list in the Bulb CMakeLists.txt
(bluegl is probably not used anyway and imageio is used in the samples for writing screenshots
images).



