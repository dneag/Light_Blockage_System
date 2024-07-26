This plugin was built to demonstrate and test the system I use to simulate light conditions in 3D space.  The classes are taken from my tree generation program, where they are
used to simulate phototropic growth in trees.  

# How it works

First, two key data structures are created:
  1.  A cubic grid, represented with a 3 dimensional array of `GridUnit`.
  2.  A generic tree where each node is a `ShadeVector`. Each represents a vector of blocked light pointing down to some grid unit, relative to an obstructed
      location on the grid.  The ShadeVectors of child nodes point to units neighboring and in the path of shade from the unit pointed to by their parent. This
      is used to propagate shade when grid units' states change.

With these established, `BlockPoint`s can be added to any location on the grid.  Adding, moving, or removing a BlockPoint will trigger shade propagation, causing
grid units within a specified range to become more or less shaded.

# Usage

* You will need Autodesk Maya 2022 installed.
* Download Light_Blockage_System.mll and place it whereever you like.
* Locate the Maya.env file at C:\Users\{you}\Documents\maya\2022\Maya.env and set the maya plugin path variable like so:
  ```
  MAYA_PLUG_IN_PATH = C:\Your\Path\To\Light_Blockage_System.mll;
* Copy or download the Tester_GUI.py script and place the file in Documents\maya\scripts
* Open Maya's script editor and execute the following:
  ```
  import importlib
  import Tester_GUI
  importlib.reload(Tester_GUI)
  Tester_GUI.GUI()
