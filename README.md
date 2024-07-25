# Overview

This plugin was built to demonstrate and test the system I use to simulate light conditions in 3D space.  The classes are taken from my tree generation program, where they are
used to simulate phototropic growth in trees.  

First, two key data structures are created:
  1.  A cubic grid, represented with a 3 dimensional array of `GridUnit`.
  2.  A generic tree where each node is a `ShadeVector`. Each represents a vector of blocked light pointing down to some grid unit, relative to an obstructed
      location on the grid.  The ShadeVectors of child nodes point to units neighboring and in the path of shade from the unit pointed to by their parent. This
      is used to propagate shade when grid units' states change.

With these established, `BlockPoint`s can be added to any location on the grid.  Adding, moving, or removing a BlockPoint will trigger shade propagation, causing
grid units within a specified range to become shaded.
