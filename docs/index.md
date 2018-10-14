# Chip8 

## Como usarlo

* `./chip_emu` - Ejecutar emulador con test.ch8 
* `./chip_emu -r <rom_name>` - Ejecutar emulador con rom_name 

## Especificaciones Chip 8
El chip 8 tiene los siguientes registros

* `uint8_t V[0x10]`: Registros de proposito general, son 16
* `uint8_t SoundTimer`: Registro que se actualiza a una frecuencia de 60Hz, en caso de llegar a cero suen un pitido
* `uint8_t DelayTimer`: Registro que se actualiza a una frecuencia de 60Hz
* `uint16_t I`: Registro que se ocupa para guardar direcciones de memoria, normalmente se utilizan los 12 bits bajos.
* `uint16_t PC`: Program Counter, se ocupara para almacenar la direccion desde donde se conseguira la siguiente instruccion
* `uint16_t SP`: Stack Pointer, Indice del ultimo elemento puesto en la stack.

### Memoria e I/O
El chip 8 tiene un `Framebuffer` de 32*64 pixeles, ocupa 16 teclas como input, Su memoria es de 0x1000 bytes
y su stack de 0x10 uint16_t

## Opcodes en chip8
Los opcodes son las *funciones* que emula
el emulador

| Opcode   | Mnemonico         | Funcion                                                     |
| -------- | -----------       | ---------                                                   |
| 0x00E0   | CLS               | Limpiar Framebuffer                                         |
| 0x00EE   | RET               | PC = Stack[SP]; SP--                                        |
| 0x1nnn   | JP nnn            | PC = nnn                                                    |
| 0x2nnn   | CALL nnn          | SP++; Stack[SP] = PC; PC = nnn                              |
| 0x3xkk   | SE Vx, kk         | if Vx == kk then PC += 2                                    |
| 0x4xkk   | SNE Vx, kk        | if Vx != kk then PC += 2                                    |
| 0x5xkk   | SE Vx, Vy         | if Vx == Vy then PC += 2                                    |
| 0x6xkk   | LD Vx, kk         | Vx = kk                                                     |
| 0x7xkk   | ADD Vx, kk        | Vx += kk                                                    |
| 0x8xy1   | OR Vx, Vy         | Vx or Vy                                                    |
| 0x8xy2   | AND Vx, Vy        | Vx &= Vy                                                    |
| 0x8xy3   | XOR Vx, Vy        | Vx ^= Vy                                                    |
| 0x8xy4   | ADD Vx, Vy        | Vx += Vy                                                    |
| 0x8xy5   | SUB Vx, Vy        | Vx -= Vy                                                    |
| 0x8xy6   | SHR Vx, (Vy >> 1) | Vx = Vy >> 1                                                |
| 0x8xy7   | SUBN Vx, Vy       | Vx = Vy - Vx                                                |
| 0x8xyE   | SHL Vx, (Vy << 1) | Vx = Vy << 1                                                |
| 0x9xy0   | SNE Vx, Vy        | if Vx != Vy then PC += 2                                    |
| 0xAnnn   | LD I, nnn         | I = nnn                                                     |
| 0xBnnn   | JP V0, nnn        | PC = V0 += nnn                                              |
| 0xCxkk   | RND Vx, kk        | Vx = rand() % kk                                            |
| 0xDxyn   | DRW Vx, Vy, n     | XOR al sprite desde la posicon I                            |
| 0xEx9E   | SKP Vx            | if Keybord[Vx] == 1 then PC += 2                            |
| 0xExA1   | SKNP Vx           | if Keyboard[Vx] == 0 then PC += 2                           |
| 0xFx07   | LD Vx, DT         | Vx = DT                                                     |
| 0xFx0A   | LD Vx, K          | Esperar una tecla y guardar el indice en Vx                 |
| 0xFx18   | LD Vx, ST         | Vx = ST                                                     |
| 0xFx1E   | ADD I, Vx         | I += Vx                                                     |
| 0xFx29   | LD F, Vx          | I = Vx * 5                                                  |
| 0xFx33   | LD B, Vx          | Guardar el valor de Vx en formato decimal a I, I + 1, i + 2 |
| 0xFx55   | LD [I], Vx        | Guardar los registros V0 hasta Vx en I                      |
| 0xFx65   | LD Vx, [I]        | Leer los registros V0 hasta Vx desde I                      |
