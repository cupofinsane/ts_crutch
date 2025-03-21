# ts_crutch
Костыль для запуска TS-ки под линуксом. 
# Как это использовать
1. Билдим:
```
g++ main.cc -lX11 -o ts_crutch
```
2. Запускаем TypeStats:
```
LANG=ru_RU wine ./TypeStats.exe
```
3. Запускаем костыль:
```
sudo ./ts_crutch
```
4. Печатаем в любом окне что угодно. Переключаемся в TS и видим захваченные нажатия!
5. Закрыть костыль - `Ctrl+C`
# Как это работает
 ts_crutch читает нажатия напрямую из файла устройства клавиатуры (поэтому нужно sudo), формирует из этих нажатий XEvent-ы и отправляет их в окно TypeStats. Как ни странно, этого достаточно. 
 Запускать непонятные кейлоггеры под `sudo` - занятие крайне сомнительное, ознакомьтесь с исходниками, прежде чем делать это.

 # Известные баги/недоработки

 - Не определяется устройство клавиатуры, на самом деле. Я захардкодкил свой сетяп. Это не дело.
 - В TS не срабатывает кнопка "Переключить в текущую раскладку" пока не вырубишь костыль.
 - Все нажатия клавиш будут отправляться в TS по два раза, если она в фокусе, поэтому не получится написать нормальное имя файла, описание для сохранённой тэ-эски, пока не вырубишь костыль. Возможно, поэтому и не срабатывает переключение в текующую раслкадку
 - Работает только под иксами, под wayland - не работает. Наверное, я не проверял. Не должно работать, мне кажется.
 - Нет мэйкфайла
 - Выход по ctrl+c - позор и отстой
 - По хорошему -  надо находить все устройства клавиатур, чтобы можно было работать за нотником с внешней клавой, например
 - Избавиться бы от sudo как-нибудь
