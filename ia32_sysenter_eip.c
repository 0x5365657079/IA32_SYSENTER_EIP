// sysenter/KiFastCallEntry/IA32_SYSENTER_EIP accrochage driver
// qunyi (https://twitter.com/0x71756e7969)

#include <wdm.h>
#include <intrin.h>

#ifndef _X86_
#error "Seul les architectures x86 sont supportées"
#endif

// Ça c'est l'adresse MSR de la valeur qui va être load dans EIP quand sysenter est appelé. Windows
// insère un pointeur dans KiFastCallEntry. Ce MSR a une portée "Core".
// https://www.felixcloutier.com/x86/sysenter
// https://xem.github.io/minix86/manual/intel-x86-and-64-manual-vol3/o_fe12b1e2a880e0ce-1350.html

#define IA32_SYSENTER_EIP 0x176

// Si vous désassemblez un service, vous verrez qu'il modifie l'index du service en EAX dans la première instruction.
// L'index est derrière le premier octet du MOV opcode et est utilisé par Sysinternals Procmon.
// 0: kd > u ZwCreateFile L4
// ntdll!NtCreateFile:
// 770655c8 b842000000      mov     eax, 42h
// 770655cd ba0003fe7f      mov     edx, offset SharedUserData!SystemCallStub(7ffe0300)
// 770655d2 ff12            call    dword ptr[edx]
// 770655d4 c22c00          ret     2Ch

#define GET_SERVICE_INDEX(Service) *((PULONG_PTR) ((PCHAR)(Service) + 1))

// On cible le syscall qu'on veut accrocher / hook

ULONG_PTR TargetSystemCallIndex;

// Valeur originale du MSR

ULONG_PTR KiFastCallEntryReal;

// Ces événements sont juste là pour garantir que l'accrochage / hook sera utilisé et cessera d'être utilisé sur tous les cores en même temps

static KEVENT EnterKiFastCallEntryHookEvent;

static KEVENT ExitKiFastCallEntryHookEvent;

// Ça c'est le stub écrit en assembleur pour instorer/restorer les régistres du CPU, vous sélectionnez le régistre, ensuite il va call le vrai KiFastCallEntryHook

extern VOID KiFastCallEntryHookStub(VOID);

// La vraie fonction va toujours être exécuté dans PASSIVE_LEVEL

VOID
NTAPI
KiFastCallEntryHook(ULONG_PTR SystemCallIndex, ULONG_PTR SavedStackPointer)
{
	if (KeReadStateEvent(&EnterKiFastCallEntryHookEvent) != 0 && KeReadStateEvent(&ExitKiFastCallEntryHookEvent) == 0)
	{
		if (SystemCallIndex == TargetSystemCallIndex)
		{
			DbgPrint("[!] KiFastCallEntryHook (eax = %p, edx = %p)\n", SystemCallIndex, SavedStackPointer);
		}
	}
}

// Procédure de déchargement de l'appareil

static
VOID
_Function_class_(DRIVER_UNLOAD)
DriverUnload(PDRIVER_OBJECT DriverObject)
{
	UNREFERENCED_PARAMETER(DriverObject);

	DbgPrint("[!] DRIVER UNLOAD\n");

	// Désactivation du crochet / hooks de signal

	KeSetEvent(&ExitKiFastCallEntryHookEvent, 0, FALSE);

	// Supprimez tous les crochets / hooks et restaurez les vrais gestionnaires du système

	KAFFINITY ActiveProcessors = KeQueryActiveProcessors();
	KAFFINITY Affinity;

	for (Affinity = 1; ActiveProcessors != 0; Affinity <<= 1, ActiveProcessors >>= 1)
	{
		if (ActiveProcessors & 1)
		{
			KeSetSystemAffinityThread(Affinity);

			DbgPrint("[!] UNHOOKING A LOGICAL PROCESSOR\n");

			__writemsr(IA32_SYSENTER_EIP, KiFastCallEntryReal);
		}
	}

	// Rétablir le fil à son affinité d'origine

	KeRevertToUserAffinityThread();
}

// EP (entry point) du driver

NTSTATUS
NTAPI
_Function_class_(DRIVER_INITIALIZE)
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(RegistryPath);

	DriverObject->DriverUnload = DriverUnload;

	DbgPrint("[!] DRIVER LOAD\n");

	// Stockez la valeur réelle du gestionnaire système de n'importe quel core

	KiFastCallEntryReal = (ULONG_PTR)__readmsr(IA32_SYSENTER_EIP);

	// Lisez l'index du service 

	TargetSystemCallIndex = GET_SERVICE_INDEX(ZwCreateFile);

	// Création des notifications

	KeInitializeEvent(&EnterKiFastCallEntryHookEvent, NotificationEvent, FALSE);

	KeInitializeEvent(&ExitKiFastCallEntryHookEvent, NotificationEvent, FALSE);

	// Épinglez le thread sur chaque noyau logique et mettez à jour son IA32_SYSENTER_EIP

	KAFFINITY ActiveProcessors = KeQueryActiveProcessors();
	KAFFINITY Affinity;

	for (Affinity = 1; ActiveProcessors != 0; Affinity <<= 1, ActiveProcessors >>= 1)
	{
		if (ActiveProcessors & 1)
		{
			KeSetSystemAffinityThread(Affinity);

			DbgPrint("[!] HOOKING A LOGICAL PROCESSOR\n");

			__writemsr(IA32_SYSENTER_EIP, (ULONG_PTR)&KiFastCallEntryHookStub);
		}
	}

	// Rétablir le fil à son affinité d'origine une seconde fois

	KeRevertToUserAffinityThread();

	// Activez l'accrochage / hook

	KeSetEvent(&EnterKiFastCallEntryHookEvent, 0, FALSE);

	return STATUS_SUCCESS;
}
