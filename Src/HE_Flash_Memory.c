/*

 Copyright (c) 2023 Hana Electronics Indústria e Comércio LTDA

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.

*/

#include "HE_Flash_Memory.h"
#include "HT_Fsm.h"
#include <string.h> // Inclua para usar memcpy

static uint8_t flash_buffer_write[1000]; // Buffer de gravação.
static uint8_t flash_buffer_read[1000];  // Buffer de leitura.

/**
 * @brief Grava um arquivo.
 *
 * A função abre o arquivo especificado em fileName em modo de gravação,
 * grava os dados do buffer fileContent no arquivo e fecha o arquivo.
 *
 * @param fileContent Ponteiro para o buffer que deve ser gravado no arquivo.
 * @param type Tipo de dado que está sendo escrito.
 * @param count Número de elementos do tipo especificado que devem ser escritos.
 * @param fileName Nome do arquivo a ser gravado.
 * @param mode Modo de abertura do arquivo.
 *
 * @return FLASH_OPERATION_OK caso a operação seja bem-sucedida ou
 *         FLASH_OPERATION_NOK caso ocorra um erro.
 */
uint8_t HE_Fs_Write(void *fileContent, uint32_t type, uint32_t count, const char *fileName, const char *mode)
{
    OSAFILE fp = PNULL;
    uint32_t len;

    // Abre o arquivo no modo especificado
    fp = OsaFopen(fileName, mode);
    if (fp == PNULL)
    {
        // Erro ao abrir o arquivo
        return FLASH_OPERATION_NOK;
    }

    // Grava os dados do buffer no arquivo
    len = OsaFwrite(fileContent, type, count, fp);

    // Sincroniza os dados gravados para garantir que estão na memória flash
    if (OsaFsync(fp) != 0)
    {
        // Erro ao sincronizar os dados
        OsaFclose(fp);
        return FLASH_OPERATION_NOK;
    }

    // Fecha o arquivo
    OsaFclose(fp);

    if (len != count)
    {
        // Erro ao gravar os dados
        return FLASH_OPERATION_NOK;
    }

    return FLASH_OPERATION_OK;
}

/**
 * @brief Lê um arquivo.
 *
 * A função abre o arquivo especificado em fileName em modo de leitura,
 * lê os dados do arquivo e os armazena em fileContent.
 *
 * @param fileContent Ponteiro para o buffer que deve ser preenchido com os dados do arquivo.
 * @param type Tipo de dado que está sendo lido.
 * @param count Número de elementos do tipo especificado que devem ser lidos.
 * @param fileName Nome do arquivo a ser lido.
 * @param mode Modo de abertura do arquivo.
 *
 * @return FLASH_OPERATION_OK caso a operação seja bem-sucedida ou
 *         FLASH_OPERATION_NOK caso ocorra um erro.
 */
uint8_t HE_Fs_Read(void *fileContent, uint32_t type, uint32_t count, const char *fileName, const char *mode)
{
    // // Limpa o buffer antes de qualquer operação de leitura
    // memset(fileContent, 0, type * count);

    // Abre o arquivo para leitura
    OSAFILE fp = OsaFopen(fileName, mode);
    if (fp == PNULL)
    {
        PRINT_LOGS('E', "[ERROR] Erro ao abrir o arquivo na flash.\n");
        return FLASH_OPERATION_NOK;
    }

    // Lê o conteúdo do arquivo
    uint32_t len = OsaFread(fileContent, type, count, fp);
    OsaFclose(fp);
    // Verifica se a leitura foi bem-sucedida
    if (len != count)
    {
        PRINT_LOGS('E', "[ERROR] Erro ao ler um arquivo da flash. len = %d | count = %d\n", len, count);
        return FLASH_OPERATION_NOK;
    }

    return FLASH_OPERATION_OK;
}

// uint8_t HE_Fs_Read(void *fileContent, uint32_t type, uint32_t count, const char *fileName, const char *mode)
// {
//     OSAFILE fp = OsaFopen(fileName, mode);
//     if (fp == PNULL)
//     {
//         printf("Erro ao abrir o arquivo para leitura\n");
//         return FLASH_OPERATION_NOK;
//     }

//     uint32_t len = OsaFread(fileContent, type, count, fp);
//     OsaFclose(fp);

//     if (len != count)
//     {
//         printf("Erro ao ler do arquivo\n");
//         return FLASH_OPERATION_NOK;
//     }

//     return FLASH_OPERATION_OK;
// }

/**
 * @brief Remove um arquivo.
 *
 * A função remove o arquivo especificado em fileName.
 *
 * @param fileName Nome do arquivo a ser removido.
 *
 * @return FLASH_OPERATION_OK caso a operação seja bem-sucedida ou
 *         FLASH_OPERATION_NOK caso ocorra um erro.
 */
uint8_t HE_Fs_File_Remove(const char *fileName)
{
    if (OsaFremove(fileName) == 0)
    {
        return FLASH_OPERATION_OK;
    }
    else
    {
        PRINT_LOGS('E', "[ERROR] Erro ao remover arquivo da flash.\n");
        return FLASH_OPERATION_NOK;
    }
}

/**
 * @brief Atualiza um arquivo com novos dados.
 *
 * A função abre o arquivo especificado em modo de leitura e escrita,
 * move o ponteiro para a posição de offset especificada e escreve os novos
 * dados no arquivo.
 *
 * @param newData Ponteiro para o buffer com os novos dados.
 * @param offset Posição no arquivo onde os novos dados devem ser escritos.
 * @param type Tipo de dado que está sendo escrito.
 * @param count Número de elementos do tipo especificado que devem ser escritos.
 * @param fileName Nome do arquivo que deve ser atualizado.
 *
 * @return FLASH_OPERATION_OK caso a operação seja bem-sucedida ou
 *         FLASH_OPERATION_NOK caso ocorra um erro.
 */
uint8_t HE_Fs_Update(void *newData, uint32_t offset, uint32_t type, uint32_t count, const char *fileName)
{
    OSAFILE fp = PNULL;
    uint32_t len;

    // Abre o arquivo no modo de leitura e escrita
    fp = OsaFopen(fileName, READ_WRITE);
    if (fp == PNULL)
    {
        // Erro ao abrir o arquivo
        return FLASH_OPERATION_NOK;
    }

    // Move o ponteiro do arquivo para a posição de offset
    if (OsaFseek(fp, offset * type, SEEK_SET) != 0)
    {
        // Erro ao mover o ponteiro
        OsaFclose(fp);
        return FLASH_OPERATION_NOK;
    }

    // Escreve os novos dados no arquivo na posição especificada
    len = OsaFwrite(newData, type, count, fp);
    OsaFsync(fp); // Sincroniza a escrita para garantir que os dados sejam gravados
    OsaFclose(fp);

    if (len != count)
    {
        return FLASH_OPERATION_NOK;
    }

    return FLASH_OPERATION_OK;
}

// Função para armazenar a mensagem Protobuf na flash e o tamanho em uma página separada.
uint8_t HE_Store_Protobuf_In_Flash(iot_DeviceToServerMessage *msg, const char *dataFileName)
{
    // Codificação da estrutura iot_DeviceToServerMessage no buffer flash_buffer_write.
    pb_ostream_t stream = pb_ostream_from_buffer(flash_buffer_write, sizeof(flash_buffer_write));

    // Codifica a mensagem no formato protobuf.
    bool status = pb_encode(&stream, iot_DeviceToServerMessage_fields, msg);
    if (!status)
    {
        PRINT_LOGS('E', "[ERROR] Erro ao codificar a mensagem Protobuf.\n");
        return FLASH_OPERATION_NOK;
    }

    // Tamanho do payload codificado.
    uint32_t encoded_size = stream.bytes_written;
    // printf("Dados codificados (hex): ");
    // for (uint32_t i = 0; i < encoded_size; i++)
    // {
    //     printf("%02x", flash_buffer_write[i]);
    // }
    // printf("\n");

    // Verifica se a página de tamanho já existe; se não, cria-a.
    uint8_t size_check = HE_Fs_Read(flash_buffer_read, sizeof(uint32_t), 1, TRACKING_DATA_SIZE_PAGE_FILENAME, READ_ONLY);
    if (size_check != FLASH_OPERATION_OK)
    {
        // Se a página não existe, cria e armazena o tamanho.
        if (HE_Fs_Write(&encoded_size, sizeof(uint32_t), 1, TRACKING_DATA_SIZE_PAGE_FILENAME, WRITE_CREATE) != FLASH_OPERATION_OK)
        {
            PRINT_LOGS('E', "[ERROR] Erro ao criar pagina de tamanhos na flash.\n");
            return FLASH_OPERATION_NOK;
        }
    }
    else
    {
        // Se a página já existe, apenas atualiza o tamanho.
        if (HE_Fs_Write(&encoded_size, sizeof(uint32_t), 1, TRACKING_DATA_SIZE_PAGE_FILENAME, WRITE_ONLY) != FLASH_OPERATION_OK)
        {
            PRINT_LOGS('E', "[ERROR] Erro ao atualizar o tamanho na pagina de tamanhos na flash.\n");
            return FLASH_OPERATION_NOK;
        }
    }
    // Armazena o payload codificado na flash.
    if (HE_Fs_Write(flash_buffer_write, sizeof(uint8_t), encoded_size, dataFileName, WRITE_CREATE) != FLASH_OPERATION_OK)
    {
        PRINT_LOGS('E', "[ERROR] Erro ao armazenar a mensagem na flash.\n");
        return FLASH_OPERATION_NOK;
    }

    PRINT_LOGS('I', "Mensagem codificada e armazenada com sucesso.\n");
    return FLASH_OPERATION_OK;
}

// Função para ler o tamanho da mensagem da página separada e depois ler a mensagem da flash.
uint8_t HE_Read_Protobuf_From_Flash(iot_DeviceToServerMessage *msg, const char *dataFileName)
{
    uint32_t encoded_size;

    // Lê o tamanho da mensagem da página separada.
    if (HE_Fs_Read(&encoded_size, sizeof(uint32_t), 1, TRACKING_DATA_SIZE_PAGE_FILENAME, READ_ONLY) != FLASH_OPERATION_OK)
    {
        PRINT_LOGS('E', "[ERROR] Erro ao ler o tamanho da mensagem da pagina de tamanho.\n");
        return FLASH_OPERATION_NOK;
    }

    // Lê a mensagem da flash para o buffer flash_buffer_read.
    if (HE_Fs_Read(flash_buffer_read, sizeof(uint8_t), encoded_size, dataFileName, READ_ONLY) != FLASH_OPERATION_OK)
    {
        PRINT_LOGS('E', "[ERROR] Erro ao ler a mensagem da flash.\n");
        return FLASH_OPERATION_NOK;
    }

    // Fluxo de entrada Protobuf para decodificação.
    pb_istream_t stream = pb_istream_from_buffer(flash_buffer_read, encoded_size);

    // Decodifica a mensagem Protobuf.
    bool status = pb_decode(&stream, iot_DeviceToServerMessage_fields, msg);
    if (!status)
    {
        PRINT_LOGS('E', "[ERROR] Erro ao decodificar a mensagem Protobuf.\n");
        return FLASH_OPERATION_NOK;
    }

    return FLASH_OPERATION_OK;
}
