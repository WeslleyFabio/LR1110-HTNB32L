# Biblioteca LR1110 para HTNB32L

[![License: Apache 2.0](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)

Esta é uma biblioteca C de minha autoria, **Weslley Fábio**, desenvolvida especificamente para facilitar a comunicação e o uso do transceptor **Semtech LR1110** (LoRa Edge™) com o microcontrolador **HTNB32L**.

O objetivo principal desta biblioteca é simplificar a integração das capacidades avançadas do LR1110 (LoRa, GNSS e Wi-Fi scanning) em projetos embarcados baseados no HTNB32L, oferecendo uma interface robusta e fácil de usar.

---

## Funcionalidades Principais

* **Comunicação SPI:** Gerencia a interface SPI para comunicação eficiente com o LR1110.
* **LoRaWAN/LoRa:** Suporte básico para o controle do rádio LoRa, permitindo transmissão e recepção de pacotes.
* **GNSS Scanning:** Funções para iniciar e gerenciar varreduras GNSS (GPS, BeiDou, Galileo, etc.) para localização.
* **Wi-Fi Scanning:** Capacidade de realizar varreduras de redes Wi-Fi para auxiliar na localização baseada em Wi-Fi.
* **Gerenciamento de Energia:** Funções para colocar o LR1110 em modos de baixa energia e otimizar o consumo.
* **Controle de Registradores:** Acesso direto e abstrato aos registradores do LR1110.
* **Gerenciamento de Interrupções:** Manipulação de interrupções geradas pelo LR1110 para eventos como conclusão de varreduras ou recepção de pacotes.

---

## Como Usar

Para usar esta biblioteca em seu projeto, siga os passos abaixo:

1.  **Clone o Repositório:**
    ```bash
    git clone https://github.com/WeslleyFabio/LR1110-HTNB32L.git
    ```

## Compatibilidade

Esta biblioteca foi desenvolvida e testada com o microcontrolador **HTNB32L**. O hardware de comunicação é baseado em **SPI**.

---

## Contribuição

Contribuições são **muito bem-vindas**! Se você encontrar bugs, tiver sugestões para novas funcionalidades, melhorias no código ou na documentação, sinta-se à vontade para:

1.  Abrir uma **Issue** para relatar problemas ou sugerir ideias.
2.  Criar um **Pull Request** com suas implementações.

---

## Licença

Este projeto é distribuído sob a licença **Apache License 2.0**. Isso significa que você é livre para usar, modificar e distribuir este software, desde que respeite os termos da licença. Uma cópia completa da licença pode ser encontrada no arquivo [`LICENSE`](LICENSE) neste repositório.

**Copyright (c) 2025 Weslley Fábio**

---

## Contato

Para dúvidas, sugestões ou apenas para dizer um "oi", você pode entrar em contato através do meu perfil no GitHub ou pelo e-mail: [weslleyfabioffs@gmail.com]

---