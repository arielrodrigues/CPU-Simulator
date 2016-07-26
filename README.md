#COMP0223: A simulator for a simple CPU (Architeture based on risk by Bruno Otavio Piedade Prado [DCOMP-UFS]) in C++

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

INSTRUÇÕES DETALHADAS:

	[Pseudo-instrução nop - 000000] Operação ociosa (U):
		E: 000
		Rz: 00000
		Rx: 00000
		Ry: 00000

	[Instrução add - 000000] Operação de adição (U): 
		E: z5x5y5
		Rz: z4z3z2z1z0
		Rx: x4x3x2x1x0
		Ry: y4y3y2y1y0
		R[z] = R[x] + R[y];
		Campo relacionado: OV

	[Instrução addi - 000001] Operação de adição imediata (F): 
		IM16: i15i14i13i12i11i10i9i8i7i6i5i4i3i2i1i0
		Rx: x4x3x2x1x0
		Ry: y4y3y2y1y0
		R[x] = R[y] + IM16;
		Campo relacionado: OV

	[Instrução sub - 000010] Operação de subtração (U): 
		E: z5x5y5
		Rz: z4z3z2z1z0
		Rx: x4x3x2x1x0
		Ry: y4y3y2y1y0
		R[z] = R[x] - R[y];
		Campo relacionado: OV

	[Instrução subi - 000011] Operação de subtração imediata (F): 
		IM16: i15i14i13i12i11i10i9i8i7i6i5i4i3i2i1i0
		Rx: x4x3x2x1x0
		Ry: y4y3y2y1y0
		R[x] = R[y] - IM16;
		Campo relacionado: OV

	[Instrução mul - 000100] Operação de multiplicação (U): 
		E: z5x5y5
		Rz: z4z3z2z1z0
		Rx: x4x3x2x1x0
		Ry: y4y3y2y1y0
		ER|R[z] = R[x] * R[y];
		Campo relacionado: OV

	[Instrução muli - 000101] Operação de multiplicação imediata (F): 
		IM16: i15i14i13i12i11i10i9i8i7i6i5i4i3i2i1i0
		Rx: x4x3x2x1x0
		Ry: y4y3y2y1y0
		ER|R[x] = R[y] * IM16;
		Campo relacionado: OV

	[Instrução div - 000110] Operação de divisão (U): 
		E: z5x5y5
		Rz: z4z3z2z1z0
		Rx: x4x3x2x1x0
		Ry: y4y3y2y1y0
		ER = R[x] mod R[y]
		R[z] = R[x] / R[y];
		Campo relacionado: OV e ZD

	[Instrução divi - 000111] Operação de divisão imediata (F): 
		IM16: i15i14i13i12i11i10i9i8i7i6i5i4i3i2i1i0
		Rx: x4x3x2x1x0
		Ry: y4y3y2y1y0
		ER = R[y] mod IM16, R[x] = R[y] / IM16;
		Campo relacionado: OV e ZD

	[Instrução cmp - 001000] Operação de comparação (U): 
		E: -x5y5
		Rx: x4x3x2x1x0
		Ry: y4y3y2y1y0
		EQ = (R[x] == R[y])
		LT = (R[x] < R[y])
		GT = (R[x] > R[y])

	[Instrução cmpi - 001001] Operação de comparação imediata (F): 
		IM16: i15i14i13i12i11i10i9i8i7i6i5i4i3i2i1i0
		Rx: x4x3x2x1x0
		EQ = (R[x] == IM16)
		LT = (R[x] < IM16)
		GT = (R[x] > IM16)

	[Instrução shl - 001010] Operação de deslocamento para esquerda (U): 
		E: z5x5y5
		Rx: x4x3x2x1x0
		Ry: y4y3y2y1y0
		ER|R[z] = ER|R[x] << (y + 1) = ER|R[x] * 2^(y+1)

	[Instrução shr - 001011] Operação de deslocamento para direita (U): 
		E: z5x5y5
		Rx: x4x3x2x1x0
		Ry: y4y3y2y1y0
		ER|R[z] = ER|R[x] >> (y + 1) = ER|R[x] / 2^(y+1)

	[Instrução and - 001100] Operador lógico AND (U): 
		E: z5x5y5
		Rx: x4x3x2x1x0
		Ry: y4y3y2y1y0
		R[z] = R[x] ^ R[y]

	[Instrução andi - 001101] Operador lógico AND imediato (F): 
		IM16: i15i14i13i12i11i10i9i8i7i6i5i4i3i2i1i0
		Rx: x4x3x2x1x0
		Ry: y4y3y2y1y0
		R[z] = R[x] ^ IM16

	[Instrução not - 001110] Operador lógico NOT (U): 
		E: -x5y5
		Rx: x4x3x2x1x0
		Ry: y4y3y2y1y0
		R[x] = ¬ R[y]

	[Instrução noti - 001111] Operador lógico NOT imediato (F): 
		IM16: i15i14i13i12i11i10i9i8i7i6i5i4i3i2i1i0
		Rx: x4x3x2x1x0
		R[x] = ¬ IM16

	[Instrução or - 010000] Operador lógico OR (U): 
		E: z5x5y5
		Rx: x4x3x2x1x0
		Ry: y4y3y2y1y0
		R[z] = R[x] v R[y]

	[Instrução ori - 010001] Operador lógico OR imediato (F): 
		IM16: i15i14i13i12i11i10i9i8i7i6i5i4i3i2i1i0
		Rx: x4x3x2x1x0
		Ry: y4y3y2y1y0
		R[x] = R[y] v IM16

	[Instrução xor - 010010] Operador lógico XOR (U): 
		E: z5x5y5
		Rx: x4x3x2x1x0
		Ry: y4y3y2y1y0
		R[z] = R[x] \oplus R[y]

	[Instrução xori - 010011] Operador lógico XOR imediato (F): 
		IM16: i15i14i13i12i11i10i9i8i7i6i5i4i3i2i1i0
		Rx: x4x3x2x1x0
		Ry: y4y3y2y1y0
		R[x] = R[y] \oplus IM16
