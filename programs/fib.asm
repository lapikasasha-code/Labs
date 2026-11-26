; fib.asm — рекурсивний Фібоначчі. Друкує fib(6) = 8.
        LOADI A, 6
        CALL  fib
        OUTN
        HALT

; fib(n): аргумент в A, результат в A. Псує B.
fib:    LOADI B, 0
        CMP   A, B
        JZ    done              ; fib(0) = 0
        LOADI B, 1
        CMP   A, B
        JZ    done              ; fib(1) = 1
        PUSH  A                 ; зберегти n
        DEC   A
        CALL  fib               ; A = fib(n-1)
        POP   B                 ; B = n
        PUSH  A                 ; зберегти fib(n-1)
        MOV   A, B              ; A = n
        DEC   A
        DEC   A
        CALL  fib               ; A = fib(n-2)
        POP   B                 ; B = fib(n-1)
        ADD   A, B              ; A = fib(n-1) + fib(n-2)
done:   RET
