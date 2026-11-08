# Autonomous Drone Rescue Mission

## Course
Computer Graphics, C++ + OpenGL + GLUT/FreeGLUT

## Project
Interactive 3D disaster-rescue simulation where the user pilots a quadcopter through a damaged environment, reaches navigation waypoints, locates a survivor, drops a medical package, and returns to the rescue helipad.

## Proposal coverage

### Member 1: Drone Design & Movement
1. Main quadcopter body
2. Four landing legs
3. Four rotor arms
4. Four propeller blades
5. Camera gimbal
6. Camera lens
7. Rescue payload box
8. Continuous propeller rotation
9. Forward movement
10. Backward movement
11. Left movement
12. Right movement
13. Takeoff animation
14. Landing animation
15. Yaw left
16. Yaw right
17. Forward flight tilt
18. Sideways turning tilt
19. Rescue package drop
20. Navigation lights + spotlight

### Member 2: Environment & Terrain
1. Ground
2. Buildings
3. Damaged buildings
4. Roads
5. Bridge
6. River
7. Trees
8. Rocks/debris
9. Rescue helipad
10. H symbol
11. Streetlights
12. Stranded vehicles
13. Clouds
14. Moving clouds
15. Sun
16. Moon
17. Day mode
18. Night mode
19. Ambient lighting
20. Sun/streetlight effects

### Member 3: Survivor & Rescue System
1. Survivor
2. Waving animation
3. Medical supply box
4. Distress signal
5. Smoke animation
6. Campfire
7. Campfire animation
8. Campfire visual lighting
9. Building collision detection
10. Ground-height restriction
11. Rescue Successful message
12. Landing zone
13. Navigation waypoints
14. Waypoints disappear after reaching
15. Hazard zones
16. Destroyed bridge
17. Flowing water
18. Rescue center
19. Emergency vehicles
20. Mission completion status

### Member 4: Camera, Weather & User Interaction
1. First-person camera
2. Third-person camera
3. Top-down camera
4. Fixed helipad camera
5. Keyboard camera switching
6. Zoom in
7. Zoom out
8. Altitude HUD
9. Speed HUD
10. Mission objective HUD
11. Minimap
12. Real-time minimap
13. Reset drone
14. Rain animation
15. Rain toggle
16. Wind flag
17. Animated flag
18. Pause
19. Resume
20. Exit

## Controls

- W/S = forward/backward
- A/D = left/right
- Q/E = yaw left/right
- R/F = altitude up/down
- 1 = First-person
- 2 = Third-person
- 3 = Top-down
- 4 = Fixed helipad camera
- +/- = zoom
- P = pause/resume
- N = day/night
- T = rain on/off
- C = spotlight on/off
- G = drop rescue package near survivor
- L = landing/takeoff
- 0 = reset mission
- H = show/hide help
- ESC = exit
- Arrow keys = movement alternative
- Mouse wheel = zoom

## Mission flow

1. Start the drone.
2. Fly through the three yellow navigation waypoints.
3. Locate the survivor at Sector A.
4. Approach the survivor and press G to drop the medical package.
5. The survivor is marked rescued after successful delivery.
6. Complete the remaining waypoint route.
7. Return to the helipad at the south side.
8. Land using L.
9. The HUD displays RESCUE SUCCESSFUL.

## Code::Blocks / Windows setup

Install FreeGLUT and OpenGL development files. In Code::Blocks, create an Empty Project and add `Autonomous_Drone_Rescue_Mission.cpp`.

Link these libraries:
- freeglut
- opengl32
- glu32

Typical linker entries on Windows:
`-lfreeglut -lopengl32 -lglu32`

If your FreeGLUT installation uses a DLL, keep `freeglut.dll` beside the generated executable.

The project intentionally uses immediate-mode OpenGL and GLUT-compatible primitives because this is a Computer Graphics course project focused on transformations, modeling, animation, lighting, camera control, interaction, depth buffering, and double buffering.
