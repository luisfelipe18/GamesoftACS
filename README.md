# GamesoftACS
This is the Anti Cheat System for Knight Online Gamesoft version
Build the application on Debugx64

The folder has two libraries inside:
 - DetourAPI
 - Virtualizer

You must add Detour3.0 and Virtualizer Include and Lib folders to VC++ Directories.
Source says that it need the Include folder of Windows SDK June 2010, but this is not available for 
Windows 8,10,11. Only Available for windows xp and vista. Link for Windows SDK June 2010: https://www.microsoft.com/en-us/download/detalles.aspx?id=8109

 - `PATH_TO_INCLUDE_FOLDER_WINDOWS_SDK_JUNE_2010; GamesoftACS/Gamesoft Security/DetourAPI/3.0/include; GamesoftACS/Gamesoft Security/Virtualizer/include`
 - `GamesoftACS/Gamesoft Security/DetourAPI/3.0/lib; GamesoftACS/Gamesoft Security/Virtualizer/lib`

If you can build this, please write me.

I have this error on compile:

![image](https://user-images.githubusercontent.com/25039923/161141679-82abb35f-7372-4d85-82a6-e87b8a7be877.png)

Compiler doesn't recognize embebbed asembler code.
