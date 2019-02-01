## Input
  - Remove loadBindings call from init
  - loadBindings should return false if no bindings file is present
  - add saveBindings
  - In editor, if loadBindings() call failed, add default bindings and call saveBindings()
  
1 / [DONE] Root motion rotation
2 / Root motion must be blended between layers too, instead of adding
3 / Blend over between source and target animations in the same layer
4 / Animation export and import

5 / [DONE] No shading on skybox
6 / [DONE] Fix empty tangents crash
7 / Deal with uppercase/lowercase resource names
8 / Directory tree should only open on double click

9 / Multiselect, select box by click+drag

10/ Asset params inspector to the right of directory view

11/ Asset preview icons where applicable

12/ Serialization/de~ for most important components (need to pass around size)
13/ Look into dynamic lib modules

14/ Extract all local resources to a subfolder on fbx import and use them as global ones?
15/ Add de/serialization for animations!

16/ Bounding boxes (maybe spheres too) for meshes!

17/ Root motion option to ignore Y coord (for root motion from pelvis and stuff like that)
	also maintain upright rotation for root and not destroy original animation while extracting
	root motion

18/ [DONE] Replace Resource::Build() with deserialize()

19/ BUG :
	INFO | 15:02:40 | 21196: Data source 'assets/materials//material3.mat' doesn't exist.
	after directory refresh

20/ Fix multiple meshes on one node for fbx to scene import

21/ 'Meta' directory for asset previews

22/ [DONE] Import animations as global resources

23/ Animator serialization
23.5/ SKIN serialization

24/ Animator editor gui animation list editing
25/ [DONE] Extract fbx skeleton to disk
26/ Handle bind poses properly

27/ Separate FBX resource extraction and scene importing