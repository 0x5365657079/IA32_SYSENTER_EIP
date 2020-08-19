# IA32_SYSENTER_EIP
Je n'ai pas développé ces directives, elles viennent du github https://gist.github.com/Barakat/7e01524fbf2a55031fb2f2d4edf1d8bd . C'est un TDT en français.<br/><br/>
<img src="https://media.discordapp.net/attachments/726930813505110029/744200647280296006/MwkS1.png"/><br/><br/>
C'est tout simplement rdmsr 176 dans Windbg en mode Kernel Debugging sur la machine locale.<br/><br/>
Cependant, le démontage de la fonction à 82c3fsd0 donne nt! ZwYieldExecution + 0aa30 plutôt que nt! KiFastCallEntry.<br/><br/>
<img src="https://media.discordapp.net/attachments/726930813505110029/744201583646212106/unknown.png"/><br/><br/>
Ça m'a laissé dans le doute, mais avec un peu de recul et de recherches j'ai pu comprendre quelque chose.<br/><br/>
Au final les instructions de montage étaient correctes, elles correspondent aux résultats de mon système de test.<br/><br/>
<img src="https://media.discordapp.net/attachments/726930813505110029/744202014111694848/unknown.png"/><br/><br/>
La solution était tout simplement : recharger les symboles.<br/><br/>
<img src="https://media.discordapp.net/attachments/726930813505110029/744202392442241044/unknown.png"/><br/>

