#!/bin/fish

# Задаем целевой размер: 10 ГБ (10 000 000 000 байт)
set TARGET_SIZE 10000000000
set FILE "bigexample.md"
set TMP_FILE "bigexample.tmp"

while test (stat -c %s $FILE) -lt $TARGET_SIZE
    # 1. Копируем текущее состояние во временный файл
    cp $FILE $TMP_FILE
    # 2. Дописываем временный файл в конец оригинального (удваиваем размер)
    cat $TMP_FILE >> $FILE
    # 3. Удаляем временный файл
    rm $TMP_FILE

    # Выводим текущий размер в консоль, чтобы видеть прогресс
    echo "Текущий размер: (stat -c %s $FILE) байт"
end

echo "Готово! Файл вырос до 10 ГБ."
