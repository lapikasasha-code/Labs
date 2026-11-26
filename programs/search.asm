; search.asm — лінійний пошук 0x41 у восьми байтах data.
; Друкує індекс першого збігу (0..7) або '-', якщо не знайшов.
        LOADH H, data           ; i = data
loop:   LOAD  A, [H]            ; A = data[i]
        LOADI B, 0x41
        CMP   A, B
        JZ    found
        INCH                    ; ++i
        HLOW                    ; A = i (data лежить на 0x800, тож молодший байт H і є індекс)
        LOADI B, 8              ; кінець ділянки
        CMP   A, B
        JZ    notfound
        JMP   loop

found:  HLOW
        OUTN
        HALT

notfound:
        LOADI A, 45             ; '-'
        OUT
        HALT

        ORG   0x800
data:   DB    7, 8, 9, 0x41, 0, 0, 0x41, 0
