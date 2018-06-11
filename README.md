### Описание
Простая файловая система по заданию. При монтировании создает 10 файлов со случайными двузначными названиями-номерами. Файлы можно только удалять. Удалять файлы нужно в порядке возрастания их номеров. При удалении "правильного" файла он просто удаляется. При удалении "неправильного" - он остается, и создаются еще 2 случайных файла.

Реализована в виде модуля ядра для ядра Linux версии 4.15. На значительно более ранних версиях, скорее всего, потребуются изменения в нескольких местах. 

### Запуск
Можно попробовать в действии на дроплете DigitalOcean'а по адресу `46.101.214.108` с подходящей версией Ubuntu. Файлы находятся в директории `/home/funfs`.

