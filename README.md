# ![logo](https://raw.githubusercontent.com/azerothcore/azerothcore.github.io/master/images/logo-github.png) AzerothCore module

# CrossFaction Battleground

### Module currently requires:
* **MODULE DON'T SUPPORT FOR AC OFFICIALLY NEED TEST THIS PR'S**
* * https://github.com/azerothcore/azerothcore-wotlk/pull/2064

## About module
This module based patch https://gist.github.com/irancore/10913800. 
But, all mechanics of change of fraction and so on is remade. Faction change occurs only for BG and nowhere else.

#### Features:
- Change you faction in bg for balance faction.

#### Config option (CFBG.conf.dist)
```ini
###################################################################################################
#   CrossFaction BattleGround
#   
#   CFBG.Enable
#       Description: Enable mixed alliance and horde in one battleground 
#       Default: 1
#
#   CFBG.Include.Avg.Ilvl.Enable
#       Description: Enable check average item level for bg
#       Default: 1
#

CFBG.Enable = 1
CFBG.Include.Avg.Ilvl.Enable = 1
```

### How to install
1. Simply place the module under the `modules` folder of your AzerothCore source folder.
2. Re-run cmake and launch a clean build of AzerothCore
3. Done :)

### Edit module configuration (optional)
If you need to change the module configuration, go to your server configuration folder (where your worldserver or `worldserver.exe` is), copy `CFBG.conf.dist` to `CFBG.conf` and edit that new file.

### Usage
- Enable system `CFBG.Enable = 1`
- Enter BG

## Credits
- [Winfidonarleyan](https://github.com/Winfidonarleyan) (Author of the module)
- [Viste](https://github.com/Viste) (Port Irancore's code to AC)
- [Irancore](https://github.com/Irancore) (Author original code for TrinityCore)
- [AzerothCore repository](https://github.com/azerothcore/azerothcore-wotlk)
- [Discord AzerothCore](https://discord.gg/PaqQRkd)
