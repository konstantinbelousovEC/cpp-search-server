# SearchServer
## Проект "Поисковый сервер"

Сервер поисковой системы, принимающий из стандартного ввода текст документов и отвечающий на поисковые запросы.

***

Мой первый проект на языке программирования С++. Его главной целью было узнать механизмы и возможности языка. При его разработке использовались встроенные типы данных; классы и структуры; шаблонные функции, перегрузки свободных функций, конструкторов, методов и операторов, специализации шаблонов; структруры данных и алгоритмы стандартной шаблонной библиотеки (в том числе их паралелльные версии); лямбда-функции; итераторы; исключения.

Также применялась практика написания юнит-тестов; работа с компиляторами clang и gcc, отладчиком GDB; использование сред разработки Xcode, VS Code, CLion. Профилирование; оценка сложности работы компонентов программы; общепринятые правила и механизмы сборки многофайловых проектов.

***

### Функционал поискового сервера

Класс [**SearchServer**](https://github.com/konstantinbelousovEC/cpp-search-server/blob/c336b8f68412285080fa056aa744067a5f40f1f2/search-server/search_server.h#L23) выполняет роль поисковго сервера и имеет следующие методы:
- Три перегруженных конструктора, принимающих в качетсве аргумента перечень стоп слов. Стоп-слова исключаются из текста документов.
    1. [*Шаблонный конструктор*](https://github.com/konstantinbelousovEC/cpp-search-server/blob/c336b8f68412285080fa056aa744067a5f40f1f2/search-server/search_server.h#L137), принимающий итерируемый контейнер из стоп-слов.
    2. [*Конструктор*](https://github.com/konstantinbelousovEC/cpp-search-server/blob/c336b8f68412285080fa056aa744067a5f40f1f2/search-server/search_server.cpp#L12) принимающий *std::string* из стоп-слов, указанных через пробел.
    3. [*Конструктор*](https://github.com/konstantinbelousovEC/cpp-search-server/blob/c336b8f68412285080fa056aa744067a5f40f1f2/search-server/search_server.cpp#L17) принимающий *std::string_view* из стоп-слов, указанных через пробел.
- Пергруженные методы *FindTopDocuments()*, возвращающие *std::vector<Document>* из не более чем [**MAX_RESULT_DOCUMENT_COUNT**](https://github.com/konstantinbelousovEC/cpp-search-server/blob/c336b8f68412285080fa056aa744067a5f40f1f2/search-server/search_server.h#L20) наиболее релевантных документов.
    1. [*Метод*]() принимающий запрос *std::string_view*.
    2. [*Метод*]() принимающий запрос *std::string_view* и категорию статуса документа в качестве предиката.
    3. [*Метод*]() принимающий запрос *std::string_view* и шаблонный предикат.
    4. А также их [*паралелльные версии*]().
- Метод [*MatchDocument()*]() в первом элементе кортежа возвращает все плюс-слова запроса, содержащиеся в документе, во втором - статус документа. Слова не дублируются и отсортированы по возрастанию. Если документ не соответствует запросу (нет пересечений по плюс-словам или есть минус-слово), вектор слов нужно возвращается пустым. Также есть [*параллельная версия метода*](https://github.com/konstantinbelousovEC/cpp-search-server/blob/0af3a5b986d3c8ac731d13c7c9db097bd96c6c10/search-server/search_server.cpp#L98).
- Метод [*GetDocumentCount()*]() возвращает количество документов в поисковой системе.
- [*GetWordFrequencies()*]() - метод получения частот слов по id документа.
- [*RemoveDocument()*]() - метод удаления документов из поискового сервера.

***

#### ConcurrentMap

Добавление в словарь — непростая операция, которая может изменить всю его структуру. Поэтому не получится увеличить количество мьютексов и распараллелить какие-либо добавления. Нужно что-то делать со словарём.

Представим, что в словаре хранятся все ключи от 1 до 10000. Интуитивно кажется: когда из одного потока  обращаетесь к ключу 10, а из другого — например, к ключу 6712, нет смысла защищать эти обращения одним и тем же мьютексом. Это отдельные области памяти, а внутреннюю структуру словаря мы никак не изменяем. А если будем обращаться к ключу 6712 одновременно из нескольких потоков, синхронизация, несомненно, понадобится.

Отсюда возникает идея: разбить словарь на несколько подсловарей с непересекающимся набором ключей и защитить каждый из них отдельным мьютексом. Тогда при обращении разных потоков к разным ключам они нечасто будут попадать в один и тот же подсловарь, а значит, смогут параллельно его обрабатывать.

Для этих целей реализован класс *ConcurrentMap*:

- [*static_assert*](https://github.com/konstantinbelousovEC/cpp-search-server/blob/0af3a5b986d3c8ac731d13c7c9db097bd96c6c10/search-server/concurrent_map.h#L13) в начале класса не даст программе скомпилироваться при попытке использовать в качестве типа ключа что-либо, кроме целых чисел.
- [*Конструктор класса ConcurrentMap<Key, Value>*](https://github.com/konstantinbelousovEC/cpp-search-server/blob/0af3a5b986d3c8ac731d13c7c9db097bd96c6c10/search-server/concurrent_map.h#L30) принимает количество подсловарей, на которые надо разбить всё пространство ключей.
- [*operator\[\]*](https://github.com/konstantinbelousovEC/cpp-search-server/blob/0af3a5b986d3c8ac731d13c7c9db097bd96c6c10/search-server/concurrent_map.h#L35) должен вести себя так же, как аналогичный оператор у map: если ключ **key** есть в словаре, должен возвращаться объект класса [*Access*](https://github.com/konstantinbelousovEC/cpp-search-server/blob/0af3a5b986d3c8ac731d13c7c9db097bd96c6c10/search-server/concurrent_map.h#L20), содержащий ссылку на соответствующее ему значение. Если **key** в словаре нет, в него надо добавить пару (**key**, **Value()**) и вернуть объект класса *Access*, содержащий ссылку на только что добавленное значение.
- Структура *Access* должна вести себя так же, как в шаблоне *Synchronized*: предоставлять ссылку на значение словаря и обеспечивать синхронизацию доступа к нему.
- Метод [*BuildOrdinaryMap*](https://github.com/konstantinbelousovEC/cpp-search-server/blob/0af3a5b986d3c8ac731d13c7c9db097bd96c6c10/search-server/concurrent_map.h#L39) должен сливать вместе части словаря и возвращать весь словарь целиком. При этом он должен быть потокобезопасным, то есть корректно работать, когда другие потоки выполняют операции с *ConcurrentMap*.

Класс *ConcurrentMap* гарантирует потокобезопасную работу со словарём. При этом он как минимум вдвое эффективнее обычного словаря с общим мьютексом.

Должно быть гарантировано, что тип *Value* имеет конструктор по умолчанию и конструктор копирования.

***

Свободная функция [*void RemoveDuplicates(SearchServer& search_server)*](https://github.com/konstantinbelousovEC/cpp-search-server/blob/c336b8f68412285080fa056aa744067a5f40f1f2/search-server/remove_duplicates.cpp#L5) для поиска и удаления дубликатов.

Дубликатами считаются документы, у которых наборы встречающихся слов совпадают. Совпадение частот необязательно. Порядок слов неважен, а стоп-слова игнорируются. При обнаружении дублирующихся документов функция должна удалить документ с большим **id** из поискового сервера, и при этом сообщить **id** удалённого документа в соответствии с форматом выходных данных.

Сложность *RemoveDuplicates()* - **O(wN(logN + logW))**, где **w** — максимальное количество слов в документе, **N** — общее количество документов, а **W** — количество слов во всех документах. 

### Сборка и установка

Скопируйте репозиторий и скомпилируйте исходные файлы либо в терминале, либо в одной из IDE.

Версия стандарта языка - C++17.
Компилятор GCC версии не ниже 9 (для компилятора clang требуются нетривиальные шаги для того, чтобы заставить его работать с параллельными алгоритмамы STL).

### Планы по доработке

Перейти на формат JSON для входных и выходных данных.