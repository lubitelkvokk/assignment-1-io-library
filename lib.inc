

section .text
 
; Принимает код возврата и завершает текущий процесс
exit: 
    xor rax, rax
    ret 

; Принимает указатель на нуль-терминированную строку, возвращает её длину
string_length:

    mov  rax, rdi
    .counter:
        cmp  byte [rdi], 0
        je   .end
        inc  rdi
        jmp  .counter
    .end:
        sub  rdi, rax
        mov  rax, rdi
    ret

; Принимает указатель на нуль-терминированную строку, выводит её в stdout
print_string:
    push rdi

    ; получаем длину строки
    call string_length
    mov rdx, rax
    ; выводим содержимое
    pop rsi ; т.к. с помощью дебаггера было выяснено, что значения callee-save регистров после вызова call заполняются 0xfff..f
    mov  rax, 1
    mov  rdi, 1
    syscall

    ret

; Принимает код символа и выводит его в stdout
print_char:

	push rdi
	mov rsi, rsp ;указываем на символ 'a', который положили в стек
	pop rdi
	mov rax, 1
	mov rdi, 1
	mov rdx, 1
	syscall

	ret

; Переводит строку (выводит символ с кодом 0xA)
; Done
print_newline:
    mov rax, 1
    mov rdi, 1
    mov rsi, 10
    mov rdx, 1
    syscall
    ret

; Выводит беззнаковое 8-байтовое число в десятичном формате 
; Совет: выделите место в стеке и храните там результаты деления
; Не забудьте перевести цифры в их ASCII коды.
print_uint:
    
    mov rax, rdi
    push rsp

    dec rsp
    mov byte[rsp], 0 ; помещаем ноль, чтобы позже знать до какого места следует считывать символы
    .loop:
        xor rdx, rdx ; перед делением необходимо обнулять значение rdx (иначе может получиться непредсказуемый результат)
        ; на выяснение этого бреда ушло минимум часа 3
        mov rdi, 10 ; указываем делитель
        div rdi 
        add dl, '0'; Добавляем код нуля

        dec rsp ; уменьшаем указатель стека для следующего символа
        mov byte[rsp], dl ; помещаем младший байт из регистра, в котором сохраняется остаток в стек

        cmp rax, 0 ; проверяем, можно ли еще осуществить деление
        je .print_number

        jmp .loop

    .print_number:
        mov rdi, rsp ; помещаем в rdi указатель на вершину стека
        call print_string ; вызываем функцию, которая будет идти по стеку и выводить символы до тех пор, пока не встретит 0
        pop rsp 
 
    ret

; Выводит знаковое 8-байтовое число в десятичном формате 
print_int:
    push rdi
    cmp rdi, 0
    jl .less
    jmp .more
    .less:
        xor rdi, rdi
        mov rdi, '-'
        call print_char
        pop rdi
        neg rdi
        call print_uint
        ret
    .more:
        call print_uint
        ret    

; Принимает два указателя на нуль-терминированные строки, возвращает 1 если они равны, 0 иначе
string_equals:

    mov r8, rdi
    mov r9, rsi
    mov rax, 0
    .loop:
        mov dil, [r8+rax]
        mov sil, [r9+rax]

        cmp dil, sil ; делаем сравнение символов по кодам. Если не равны, выход с 0 кодом
        jne .error

        cmp dil, 0 ; теперь когда символы точно равны, проверяем, конец ли это строки и выходим с кодом 1
        je .equal 

        inc rax ; если ни то ни другое условие не выполнилось, то переходим к следующему символу, увеличивая адрес
        jmp .loop
        
    .error:
        mov rax, 0
        ret
    .equal:
        mov rax, 1
        ret

; Читает один символ из stdin и возвращает его. Возвращает 0 если достигнут конец потока
read_char:

    mov rax, 0 ; системный вызов read
    mov rdi, 0 ; считываение из stdin
    lea rsi, [rsp-1] ; запишем наш символ в ячейке над вершиной стека(lea получит адрес rsp-1)
    mov rdx, 1 ; считываем 1 символ
    syscall

    cmp rax, 0 ; показывает количество прочитанных символов(если равно 0, то мы достигли конца потока)
    ; это нельзя заменить на проверку символа в ячейке rsp-1, т.к. туда ничего не записалось, и, возможно, там находилось значение
    ; отличное от 0
    je .null
    

    mov al, byte[rsp-1] ; загружаем прочитанный символ в al (младший байт rax)
    ret

    .null:
        ret

; Принимает: адрес начала буфера, размер буфера
; Читает в буфер слово из stdin, пропуская пробельные символы в начале, .
; Пробельные символы это пробел 0x20, табуляция 0x9 и перевод строки 0xA.
; Останавливается и возвращает 0 если слово слишком большое для буфера
; При успехе возвращает адрес буфера в rax, длину слова в rdx.
; При неудаче возвращает 0 в rax
; Эта функция должна дописывать к слову нуль-терминатор

read_word:

    push rdi
    push rsi
    
    push rdi
    push rsi

    .unnecessary_characters: ; считываем символы до тех пор, пока не встретится символ помимо пробела, табуляции или перевода строки
        xor rax, rax
        call read_char

        cmp rax, 0x20
        je .unnecessary_characters

        cmp rax, 0x9
        je .unnecessary_characters

        cmp rax, 0xA
        je .unnecessary_characters

    .reading_word:
        ; На этом этапе у нас имеет прочитанный символ
        
        pop rsi
        pop rdi
        cmp rax, 0x20
        je .exit

        cmp rax, 0x9
        je .exit

        cmp rax, 0
        je .exit

        cmp rax, 0xA
        je .exit
    
        cmp rsi, 0
        je .err_exit

        mov byte[rdi], al ; записываем прочитанный символ
        inc rdi ; перемещаем указатель на следующий байт
        dec rsi ; уменьшаем размер буфера

        push rdi ; после вызова функции эти регистры изменятся, поэтому сохраним их
        push rsi
        call read_char
        pop rsi
        pop rdi

        push rdi
        push rsi
        jmp .reading_word

    .exit:
        mov byte[rdi], 0
        pop rdx ; получаем изначальную длину буфера
        sub rdx, rsi ; вычисляем длину слова
        pop rax ; получаем адрес начала буфера
        ret

    .err_exit:
        pop rsi
        pop rdi
        xor rax, rax
        ret

 

; Принимает указатель на строку, пытается
; прочитать из её начала беззнаковое число.
; Возвращает в rax: число, rdx : его длину в символах
; rdx = 0 если число прочитать не удалось
parse_uint:
    xor rax, rax
    ret




; Принимает указатель на строку, пытается
; прочитать из её начала знаковое число.
; Если есть знак, пробелы между ним и числом не разрешены.
; Возвращает в rax: число, rdx : его длину в символах (включая знак, если он был) 
; rdx = 0 если число прочитать не удалось
parse_int:
    xor rax, rax
    ret 

; Принимает указатель на строку, указатель на буфер и длину буфера
; Копирует строку в буфер
; Возвращает длину строки если она умещается в буфер, иначе 0
string_copy:
    xor rax, rax
    ret


