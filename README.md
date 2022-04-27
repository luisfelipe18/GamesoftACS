# GamesoftACS
This is the Anti Cheat System for Knight Online Gamesoft version
Build the application on Debugx84

The folder has three libraries inside:
 - DetourAPI
 - Virtualizer
 - Windows SDK June 2010 (This library was deprecated and cannot be installed on modern systems like Windows 10, Windows 8, Windows 11, I have found a Legacy version which installation is problematic but we only need the `Lib` and `Include` folders. I have added those folders to project to make it easy to build for everyone)

The Project has relative paths to DetoursAPI, Virtualizer, WindowsSDK `include` and `lib` folders, so you dont need to change anything.

![image](https://user-images.githubusercontent.com/25039923/165550690-d9d0c82c-de8e-456e-a005-ef515f835db2.png)

This is the output:
![image](https://user-images.githubusercontent.com/25039923/165551106-a110d9b9-c8ba-4d28-bedc-f280aa482fac.png)

But I dont known what to do with this output 🤣🤣

**Note:**
When building, don't forget to change the IP addres on lines 394 and 410 from `Pearl Engine.cpp` file by your Server IP address.

**If you can build in Debugx64, please contact me or send a pull request.**
