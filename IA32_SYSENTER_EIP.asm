; sysenter/KiFastCallEntry/IA32_SYSENTER_EIP hooking driver
; qunyi (https://twitter.com/0x71756e7969)

.model flat, stdcall

KiFastCallEntryHook proto near stdcall,
	SystemCallIndex:dword,
	SavedStackPointer:dword

extern KiFastCallEntryReal: dword

public KiFastCallEntryHookStub

.code

KiFastCallEntryHookStub proc

  ; À ce stade, les sélecteurs de segment cs et ss ont des sélecteurs de kernel-mode (ring 0)
	; (IA32_SYSENTER_CS, IA32_SYSENTER_CS + 8), et stack pointer (ESP)
	; Pour valider la stack du Kernel (IA32_SYSENTER_ESP). On doit instorer des registers 
  ; donc on peut restaurer leur contenu après avoir appelé notre hool et corriger les sélecteurs restants
	; car ils utilisent toujours des sélecteurs user-mode (ring 3)

	; Stockez les registres généraux et les EFLAGS sur la pile

	pushad
	pushfd

	push fs
	push ds
	push es

	; Les sélecteurs de segment (fs, ds, es) contiendront des sélecteurs du user-mode 
	; donc nous devons les définir sur les sélecteurs du kernel-mode(fs = 0x30, ds = es = 0x23)

	push cx

	; Load 0x30 dans sélecteur fs segment 
	mov cx, 30h
	mov fs, cx

	; Load 0x23 dans ds et es segment sélecteurs 
	mov cx, 23h
	mov ds, cx
	mov es, cx

	pop cx

	; Appelez le hook et passez l'index du syscall (eax) et le pointeur de pile utilisateur (edx)

	invoke KiFastCallEntryHook, eax, edx

	; On restaure les valuers initiales des registres 

	pop es
	pop ds
	pop fs

	popfd
	popad

	; Et pour fini on jump au vrai gestionnaire système
	jmp [KiFastCallEntryReal]

KiFastCallEntryHookStub endp

end
