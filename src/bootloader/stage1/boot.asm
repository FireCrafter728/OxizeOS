org 0x7C00
bits 16


%define ENDL 0x0D, 0x0A

jmp short start
nop

bdb_oem:                    times 8 db 0
bdb_bytes_per_sector:       dw 0
bdb_sectors_per_cluster:    db 0
bdb_reserved_sectors:       dw 0
bdb_fat_count:              db 0
bdb_dir_entries_count:      dw 0
bdb_total_sectors:          dw 0
bdb_media_descriptor_type:  db 0
bdb_sectors_per_fat:        dw 0
bdb_sectors_per_track:      dw 0
bdb_heads:                  dw 0
bdb_hidden_sectors:         dd 0
bdb_large_sector_count:     dd 0

fat32_sectors_per_fat:      dd 0
fat32_flags:                dw 0
fat32_fat_version_number:   dw 0
fat32_rootdir_cluster:      dd 0
fat32_fsinfo_sector:        dw 0
fat32_backup_boot_sector:   dw 0
fat32_reserved:             times 12 db 0
fat32_drive_number:         db 0
fat32_nt_flags:             db 0
fat32_sig:                  db 0
fat32_disk_serial_number:   dd 0
fat32_volume_label:         times 11 db 0
fat32_sysid:                times 8 db 0

start:
    mov ah, 0x0e
    mov al, 0x0A
    int 0x10
    ; cli
    ; hlt
    mov ax, PARTITION_ENTRY_SEGMENT
    mov es, ax
    mov di, PARTITION_ENTRY_OFFSET
    mov cx, 16
    rep movsb

    mov ax, 0
    mov ds, ax
    mov es, ax
    
    mov ss, ax
    mov sp, 0x7C00

    push es
    push word .after
    retf

.after:

    ; read something from floppy disk
    ; BIOS should set DL to drive number
    mov [fat32_drive_number], dl

    mov ah, 0x41
    mov bx, 0x55AA
    stc
    int 0x13

    jc .no_disk_extensions
    cmp bx, 0xAA55
    jne .no_disk_extensions

    mov byte [have_extensions], 1
    jmp .after_disk_extensions_check
    
.no_disk_extensions:
    mov byte [have_extensions], 0

.after_disk_extensions_check:

    mov si, stage2_location

    mov ax, STAGE2_LOAD_SEGMENT         ; set segment registers
    mov es, ax

    mov bx, STAGE2_LOAD_OFFSET

.loop:
    mov eax, [si]
    add si, 4
    mov cl, [si]
    inc si

    cmp eax, 0
    je .read_finish

    call disk_read

    xor ch, ch
    shl cx, 5
    mov di, es
    add di, cx
    mov es, di

    jmp .loop

.read_finish:
    
    ; jump to our kernel
    mov dl, [fat32_drive_number]          ; boot device in dl
    mov si, PARTITION_ENTRY_OFFSET
    mov di, PARTITION_ENTRY_SEGMENT

    mov ax, STAGE2_LOAD_SEGMENT         ; set segment registers
    mov ds, ax
    mov es, ax

    jmp STAGE2_LOAD_SEGMENT:STAGE2_LOAD_OFFSET

    jmp wait_key_and_reboot             ; should never happen

    cli                                 ; disable interrupts, this way CPU can't get out of "halt" state
    hlt


;
; Error handlers
;

floppy_error:
    mov si, msg_read_failed
    call puts
    jmp wait_key_and_reboot

wait_key_and_reboot:
    mov ah, 0
    int 16h                     ; wait for keypress
    jmp 0FFFFh:0                ; jump to beginning of BIOS, should reboot

    cli                         ; disable interrupts, this way CPU can't get out of "halt" state
    hlt


;
; Prints a string to the screen
; Params:
;   - ds:si points to string
;
puts:
    ; save registers we will modify
    push si
    push ax
    push bx

.loop:
    lodsb               ; loads next character in al
    or al, al           ; verify if next character is null?
    jz .done

    mov ah, 0x0E        ; call bios interrupt
    mov bh, 0           ; set page number to 0
    int 0x10

    jmp .loop

.done:
    pop bx
    pop ax
    pop si    
    ret

;
; Disk routines
;

;
; Converts an LBA address to a CHS address
; Parameters:
;   - ax: LBA address
; Returns:
;   - cx [bits 0-5]: sector number
;   - cx [bits 6-15]: cylinder
;   - dh: head
;

lba_to_chs:

    push ax
    push dx

    xor dx, dx                          ; dx = 0
    div word [bdb_sectors_per_track]    ; ax = LBA / SectorsPerTrack
                                        ; dx = LBA % SectorsPerTrack

    inc dx                              ; dx = (LBA % SectorsPerTrack + 1) = sector
    mov cx, dx                          ; cx = sector

    xor dx, dx                          ; dx = 0
    div word [bdb_heads]                ; ax = (LBA / SectorsPerTrack) / Heads = cylinder
                                        ; dx = (LBA / SectorsPerTrack) % Heads = head
    mov dh, dl                          ; dh = head
    mov ch, al                          ; ch = cylinder (lower 8 bits)
    shl ah, 6
    or cl, ah                           ; put upper 2 bits of cylinder in CL

    pop ax
    mov dl, al                          ; restore DL
    pop ax
    ret


;
; Reads sectors from a disk
; Parameters:
;   - ax: LBA address
;   - cl: number of sectors to read (up to 128)
;   - dl: drive number
;   - es:bx: memory address where to store read data
;
disk_read:

    push eax                             ; save registers we will modify
    push bx
    push cx
    push dx
    push si
    push di

    cmp byte [have_extensions], 1
    jne .no_disk_extensions

    mov [extensions_dap.lba], eax
    mov [extensions_dap.segment], es
    mov [extensions_dap.offset], bx
    mov [extensions_dap.count], cl

    mov ah, 0x42
    mov si, extensions_dap
    mov di, 3
    jmp .retry

.no_disk_extensions:
    push cx                             ; temporarily save CL (number of sectors to read)
    call lba_to_chs                     ; compute CHS
    pop ax                              ; AL = number of sectors to read
    
    mov ah, 02h
    mov di, 3                           ; retry count

.retry:
    pusha                               ; save all registers, we don't know what bios modifies
    stc                                 ; set carry flag, some BIOS'es don't set it
    int 13h                             ; carry flag cleared = success
    jnc .done                           ; jump if carry not set

    ; read failed
    popa
    call disk_reset

    dec di
    test di, di
    jnz .retry

.fail:
    ; all attempts are exhausted
    jmp floppy_error

.done:
    popa

    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop eax                             ; restore registers modified
    ret


;
; Resets disk controller
; Parameters:
;   dl: drive number
;
disk_reset:
    pusha
    mov ah, 0
    stc
    int 13h
    jc floppy_error
    popa
    ret

msg_read_failed:        db 'Read from disk failed!', ENDL, 0

have_extensions:        db 0
extensions_dap:
    .size:              db 0x10
                        db 0
    .count:             dw 0
    .offset:            dw 0
    .segment:           dw 0
    .lba:               dq 0

STAGE2_LOAD_SEGMENT     equ 0x0
STAGE2_LOAD_OFFSET      equ 0x500

PARTITION_ENTRY_SEGMENT equ 0x2000
PARTITION_ENTRY_OFFSET  equ 0x0

msg_test:               db 'Hello, world!', ENDL, 0

times 480-($-$$)        db 0

stage2_location:        db 1
times 29 db 0
dw 0xAA55