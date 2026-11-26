; hello.asm — друкує рядок із регіону даних
        OUTS  msg
        HALT

        ORG   0x800
msg:    DB    72, 69, 76, 76, 79, 0     ; "HELLO", 0
