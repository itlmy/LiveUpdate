.model flat

.data
__imp__GetLogicalProcessorInformation@4 dd dummy
EXTERNDEF __imp__EncodePointer@4 : DWORD

.code
dummy proc
mov eax, [esp+4]
ret 4
dummy endp

end