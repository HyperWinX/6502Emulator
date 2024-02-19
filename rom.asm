.org $0200

start:
	ldx #0
iter:
	lda msg, X
	sta $FFFB
	inx
	cpx #0
	beq end
	jmp iter
end:
	kil

msg:
	.byte "Message from 6502 emulator!", 0
