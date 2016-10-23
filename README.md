#A simulator for a simple CPU in C++
Architeture by Bruno Otavio Piedade Prado [DCOMP-UFS] - for details about architeture contact Bruno Otavio Piedade Prado in https://admin.dcomp.ufs.br/perfil/13227/

(Sorry, only br portuguese below)

---------

FORMATOS DE INSTRUÇÕES

	Tipo U: 
		6 bits para operação (OP: 31-26)
		3 bits para extensão (E: 17-15)
		5 bits para operandos (Rz: 14-10, Rx: 9-5, Ry:4-0)

	Tipo F:
		6 bits para operação (OP: 31-26)
		16 bits para valor imediato (IM16: 25-10)
		5 bits para operandos (Rx: 9-5, Ry: 4-0)

	Tipo S:
		6 bits para operação (OP: 31-26)
		26 bits para valor imediato (IM26: 25-0)

------

INSTRUÇÕES:

-> Arithmetic and logic operations

	Adição (add, addi)
		add -> code: 0
		addi -> code: 1

	Subtração (sub, subi)
		add -> code: 2
		addi -> code: 3

	Multiplicação (mul, muli)
		add -> code: 4
		addi -> code: 5

	Divisão (div, divi)
		add -> code: 6
		addi -> code: 7

	Comparação (cmp, cmpi)
		add -> code: 8
		addi -> code: 9

	Deslocamento (shl, shr)
		add -> code: 10
		addi -> code: 11

	Lógicas (and, andi)
		add -> code: 12
		addi -> code: 13

	Lógicas (not, noti)
		add -> code: 14
		addi -> code: 15

	Lógicas (or, ori)
		add -> code: 16
		addi -> code: 17

	Lógicas (xor, xori)
		add -> code: 18
		addi -> code: 19

-> Memory access operations

	Leitura/escrita de palavras/bytes (ldw, ldb, stw, stb)
		ldw -> code: 20
		ldb -> code: 21
		stw -> code: 22
		sdb -> code: 23

-> Control flow operations

	Desvio incondicional (bun):
		bun -> code: 26

	Desvio condicional (beq, blt, bgt, bne, ble, bge):
		beq -> code: 27
		blt -> code: 28
		bgt -> code: 29
		bne -> code: 30
		ble -> code: 31
		bge -> code: 32

	Interrupção (int):
		int -> code: 64


-------

Unidade de ponto flutuante (fpu) mapeada nas posições 0x8800, 0x8804, 0x8808, 0x880C da memória.

Terminal mapeado na posição 0x888B da memória.

Watchdog mapeado na posição 0x8080 da memória.

Arquivos de teste disponíveis em /files4test/ 
