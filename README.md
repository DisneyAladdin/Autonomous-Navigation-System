# Autonomous-Navigation-System
This program is used for autonomous navigation system using a DGPS

# Contents
InputFile : test1.txt (MAP for the autonomous navigation indicated by plane cordinate (X,Y))

MainProgram : navigation.c (Execute the autonomous navigation)

SubProgram : DGPS.c (Calculate the position of the mobile robot and convert the postioning data into plane cordinate)

OutputFile : test2.text (Trajectory of the autonomous navigation)


# Algorithm
1. The mobile robot achives the autonomous navigation by approaching the targets(waypoints) from start to the goal

2. The robot mainly calculates its position by encoder connected to right and left wheels.

3. If the distance between the robot and the target is less than 0.5m, change target to next waypoint.

4. In order to modify the positioning error caused by wheel slippage, we use DGPS(Differential Global Positioning System).

5. The DGPS's positioning data is shared with MainProgram(navigation.c) by share memory.

# Licence 
CopyRight (c) 2018 Shuto Kawabata

Released under the MIT licence

https://opensource.org/licenses/MIT


# Author
Shuto Kawabata
