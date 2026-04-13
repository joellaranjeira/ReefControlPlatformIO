# Project Guidelines

## Code Style
C++ estilo Arduino (funções setup/loop, defines para constantes, comentários em português/emoji). Nomes de variáveis em português (ex.: `ultimaTemperatura`, `buzzerManual`). Uso de millis() para temporização não-bloqueante. Veja [src/main.cpp](src/main.cpp) para exemplos.

## Architecture
Sistema de monitoramento e controle para aquário marinho, focado em alertas de temperatura. Componentes: ESP32, sensor DS18B20, LEDs, buzzer, integração com Telegram Bot e Arduino IoT Cloud. Limites: sensor isolado, alertas independentes de comunicação. Veja [README.md](README.md) para funcionalidades.

## Build and Test
Compilação: `pio run`. Upload: `pio run --target upload`. Monitor: `pio device monitor`. Não há testes automatizados; testes manuais via hardware e monitor serial.

## Conventions
Credenciais em [src/secrets.h](src/secrets.h) (não commite). Propriedades IoT em [src/thingProperties.h](src/thingProperties.h). Versão do firmware em define (FW_VERSION). Organização: src/ para código, include/lib para headers/libs, test/ para testes (atualmente vazio).</content>
<parameter name="filePath">c:\Users\Laranjeira\OneDrive\Documentos\PlatformIO\Projects\ReefControlPlatformIO\.github\copilot-instructions.md