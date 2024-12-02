This plugin was built to demonstrate and test the system I use to simulate light conditions in 3D space.  The classes are taken from my tree generation program, where they are
used to simulate phototropic growth in trees.  

# How it works

First, two key data structures are created:
  1.  A cubic grid, represented with a 3 dimensional array of `GridUnit`.
  2.  A directed acyclic graph where each node is a `ShadeVector`. Each represents a vector of blocked light pointing down to some grid unit, from an obstructed
      location on the grid.  The ShadeVectors of child nodes point to units neighboring and in the same obstructed path from the unit pointed to by their parent. When conditions inside a grid unit change, i.e. obstructing objects are added or removed, shade is propagated along these paths.

With these established, `BlockPoint`s can be added to any location on the grid.  Adding, moving, or removing a BlockPoint will trigger shade propagation, causing
grid units within a specified range to become more or less shaded.

## Key Classes

The source code contains a number of utility and debugging classes and methods, so I will highlight here those that are responsible for the core functionality.
* `BlockPointGrid`
  * `initiateGrid()`
  * `createShadeVectorGraph()`
  * `applyShade()`
* `GridUnit`
* `BlockPoint`
* `ShadeVector`
 

## Usage

* If you have or can download and install Maya 2022, then you can just download Light_Blockage_System.mll and follow the instructions below.  If you would like
  to use a different version, you will need to create a Visual Studio project configured to build Maya plugins with your version.  I recommend following this [tutorial](https://www.youtube.com/watch?v=fBGrdoN4roE)
  to set that up.  It is for Maya 2022, but you can likely just substitute your version where necessary.  Once set up, check out the repository, copy the source files to your project, and build. 
* Locate the Maya.env file at C:\Users\{you}\Documents\maya\2022\Maya.env and set the maya plugin path variable like so:
  ```
  MAYA_PLUG_IN_PATH = C:\Your\Path\To\Light_Blockage_System.mll;
* Go to Windows -> Settings and Preferences -> Plug-in Manager.  Locate the plugin and ensure the 'Loaded' checkbox is checked.
* Copy or download the Tester_GUI.py script and place the file in Documents\maya\scripts
* Open Maya's script editor and execute the following:
  ```
  import importlib
  import Tester_GUI
  importlib.reload(Tester_GUI)
  Tester_GUI.GUI()
* To see the transparency change in affected grid units, download transparency_tile_map_0-100.jpg, create a material, and use the jpg as the material's transparency map.  The name of the material
  must match the hardcoded shading group name in `BlockPointGrid::initiateGrid`.  This is set as "shadePercentageMat".

