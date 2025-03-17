#include <iostream>
#include <list>
#include <string>
#include <algorithm>
#include <fstream>

using namespace std;

enum class Type : uint8_t {
    INT,
    DOUBLE,
    STRING,
    CHAR
};

int getTypeSize(Type type) {
    switch (type) {
        case Type::INT: return 4;
        case Type::DOUBLE: return 8;
        case Type::STRING: return 20;
        case Type::CHAR: return 1;
        default: return 0;
    }
}

struct Attribute {
    int id;
    string name;
    Type type;
    int size;
    bool isPrimaryKey;
    bool isForeignKey;

    Attribute(int i, string n, Type t, bool pk, bool fk)
            : id(i), name(move(n)), type(t), size(getTypeSize(t)), isPrimaryKey(pk), isForeignKey(fk) {}
};

bool compareAttributes(const Attribute& a1, const Attribute& a2) {
    return a1.size > a2.size;
}

class RelationType {
public:
    string name;
    int rowSize;
    list<Attribute> attributes;

    RelationType(string n) : name(move(n)), rowSize(0) {}

    void addAttribute(const Attribute& attr) {
        attributes.push_back(attr);
        rowSize += attr.size;
        attributes.sort(compareAttributes);
    }
};

class Table {
public:
    string name;
    string storageFile;
    list<pair<string, int>> attributeInfo;

    Table(string n, string file)
            : name(move(n)), storageFile(move(file)) {}

    void addAttributeInfo(const string& attrName, int attrId) {
        attributeInfo.emplace_back(attrName, attrId);
    }
};

class TableList {
public:
    RelationType* type;
    list<Table> tables;

    TableList(RelationType* t) : type(t) {}

    void addTable(const Table& table) {
        tables.push_back(table);
    }
};

void LOADT(const string& filename, const list<TableList*>& tableLists) {
    ofstream outFile(filename, ios::binary);
    if (!outFile) {
        cerr << "Unable to open file!" << endl;
        return;
    }

    size_t numTypes = tableLists.size();
    outFile.write(reinterpret_cast<const char*>(&numTypes), sizeof(numTypes));

    for (const auto& tableList : tableLists) {
        outFile.write(reinterpret_cast<const char*>(&tableList->type->rowSize), sizeof(tableList->type->rowSize));

        size_t typeNameLength = tableList->type->name.size();
        outFile.write(reinterpret_cast<const char*>(&typeNameLength), sizeof(typeNameLength));
        outFile.write(tableList->type->name.data(), typeNameLength);

        size_t numAttributes = tableList->type->attributes.size();
        outFile.write(reinterpret_cast<const char*>(&numAttributes), sizeof(numAttributes));

        for (const auto& attr : tableList->type->attributes) {
            outFile.write(reinterpret_cast<const char*>(&attr.id), sizeof(attr.id));

            size_t nameLength = attr.name.size();
            outFile.write(reinterpret_cast<const char*>(&nameLength), sizeof(nameLength));
            outFile.write(attr.name.c_str(), nameLength);

            outFile.write(reinterpret_cast<const char*>(&attr.type), sizeof(attr.type));
            outFile.write(reinterpret_cast<const char*>(&attr.size), sizeof(attr.size));
            outFile.write(reinterpret_cast<const char*>(&attr.isPrimaryKey), sizeof(attr.isPrimaryKey));
            outFile.write(reinterpret_cast<const char*>(&attr.isForeignKey), sizeof(attr.isForeignKey));
        }

        size_t numTables = tableList->tables.size();
        outFile.write(reinterpret_cast<const char*>(&numTables), sizeof(numTables));

        for (const auto& table : tableList->tables) {
            size_t nameLength = table.name.size();
            outFile.write(reinterpret_cast<const char*>(&nameLength), sizeof(nameLength));
            outFile.write(table.name.c_str(), nameLength);

            size_t fileLength = table.storageFile.size();
            outFile.write(reinterpret_cast<const char*>(&fileLength), sizeof(fileLength));
            outFile.write(table.storageFile.c_str(), fileLength);

            size_t numAttributes = table.attributeInfo.size();
            outFile.write(reinterpret_cast<const char*>(&numAttributes), sizeof(numAttributes));

            for (const auto& attrInfo : table.attributeInfo) {
                size_t attrNameLength = attrInfo.first.size();
                outFile.write(reinterpret_cast<const char*>(&attrNameLength), sizeof(attrNameLength));
                outFile.write(attrInfo.first.c_str(), attrNameLength);

                outFile.write(reinterpret_cast<const char*>(&attrInfo.second), sizeof(attrInfo.second));
            }
        }
    }
    outFile.close();
}

list<TableList*> RECOVERYT(const string& filename) {
    ifstream inFile(filename, ios::binary);
    if (!inFile) {
        cerr << "Unable to open file!" << endl;
        return {};
    }

    list<TableList*> tableLists;

    size_t numTypes;
    inFile.read(reinterpret_cast<char*>(&numTypes), sizeof(numTypes));

    for (size_t i = 0; i < numTypes; ++i) {
        int rowSize;
        inFile.read(reinterpret_cast<char*>(&rowSize), sizeof(rowSize));

        size_t typeNameLength;
        inFile.read(reinterpret_cast<char*>(&typeNameLength), sizeof(typeNameLength));
        string name(typeNameLength, '\0');
        inFile.read(&name[0], typeNameLength);

        auto* type = new RelationType(name);
        type->rowSize = rowSize;

        size_t numAttributes;
        inFile.read(reinterpret_cast<char*>(&numAttributes), sizeof(numAttributes));

        for (size_t j = 0; j < numAttributes; ++j) {
            int id;
            inFile.read(reinterpret_cast<char*>(&id), sizeof(id));

            size_t nameLength;
            inFile.read(reinterpret_cast<char*>(&nameLength), sizeof(nameLength));
            string name(nameLength, '\0');
            inFile.read(&name[0], nameLength);

            Type typeEnum;
            inFile.read(reinterpret_cast<char*>(&typeEnum), sizeof(typeEnum));

            int size;
            bool isPrimaryKey, isForeignKey;
            inFile.read(reinterpret_cast<char*>(&size), sizeof(size));
            inFile.read(reinterpret_cast<char*>(&isPrimaryKey), sizeof(isPrimaryKey));
            inFile.read(reinterpret_cast<char*>(&isForeignKey), sizeof(isForeignKey));

            type->addAttribute(Attribute(id, name, typeEnum, isPrimaryKey, isForeignKey));
        }

        auto* tableList = new TableList(type);

        size_t numTables;
        inFile.read(reinterpret_cast<char*>(&numTables), sizeof(numTables));

        for (size_t j = 0; j < numTables; ++j) {
            size_t nameLength;
            inFile.read(reinterpret_cast<char*>(&nameLength), sizeof(nameLength));
            string name(nameLength, '\0');
            inFile.read(&name[0], nameLength);

            size_t fileLength;
            inFile.read(reinterpret_cast<char*>(&fileLength), sizeof(fileLength));
            string storageFile(fileLength, '\0');
            inFile.read(&storageFile[0], fileLength);

            Table table(name, storageFile);

            size_t numAttributes;
            inFile.read(reinterpret_cast<char*>(&numAttributes), sizeof(numAttributes));

            for (size_t k = 0; k < numAttributes; ++k) {
                size_t attrNameLength;
                inFile.read(reinterpret_cast<char*>(&attrNameLength), sizeof(attrNameLength));
                string attrName(attrNameLength, '\0');
                inFile.read(&attrName[0], attrNameLength);

                int attrId;
                inFile.read(reinterpret_cast<char*>(&attrId), sizeof(attrId));

                table.addAttributeInfo(attrName, attrId);
            }
            tableList->addTable(table);
        }
        tableLists.push_back(tableList);
    }
    inFile.close();
    return tableLists;
}

void GetR(const string& relationName, const list<TableList*>& tableLists) {
    for (const auto& tableList : tableLists) {
        for (const auto& table : tableList->tables) {
            if (table.name == relationName) {
                cout << "Relation name: " << relationName << endl;
                cout << "  File: " << table.storageFile << endl;
                cout << "  Attributes:" << endl;

                const auto& attributes = tableList->type->attributes;

                for (const auto& attrInfo : table.attributeInfo) {
                    int attrId = attrInfo.second;
                    auto it = find_if(attributes.begin(), attributes.end(),
                                      [attrId](const Attribute& attr) { return attr.id == attrId; });
                    if (it != attributes.end()) {
                        const auto& attr = *it;
                        cout << "    Name: " << attrInfo.first
                             << ", ID: " << attr.id
                             << ", Type: " << static_cast<int>(attr.type)
                             << ", Size: " << attr.size
                             << ", PK: " << attr.isPrimaryKey
                             << ", FK: " << attr.isForeignKey
                             << endl;
                    }
                }
                return;
            }
        }
    }
    cout << "Relation with name " << relationName << " not found." << endl;
}

int main() {
    RelationType type1("Type1");
    type1.addAttribute(Attribute(1, "id", Type::INT, true, false));
    type1.addAttribute(Attribute(2, "name", Type::STRING, false, false));
    type1.addAttribute(Attribute(3, "email", Type::STRING, false, false));

    Table table1("Users", "users.bin");
    table1.addAttributeInfo("id", 1);
    table1.addAttributeInfo("name", 2);
    table1.addAttributeInfo("email", 3);

    TableList tableList1(&type1);
    tableList1.addTable(table1);

    RelationType type2("Type2");
    type2.addAttribute(Attribute(1, "id", Type::INT, true, false));
    type2.addAttribute(Attribute(2, "user_id", Type::INT, false, true));

    Table table2("Orders", "orders.bin");
    table2.addAttributeInfo("id", 1);
    table2.addAttributeInfo("user_id", 2);

    TableList tableList2(&type2);
    tableList2.addTable(table2);

    RelationType type3("Type3");
    type3.addAttribute(Attribute(1, "id", Type::INT, true, false));
    type3.addAttribute(Attribute(2, "order_id", Type::INT, false, true));
    type3.addAttribute(Attribute(3, "sauce", Type::CHAR, false, false));
    type3.addAttribute(Attribute(4, "price", Type::DOUBLE, false, false));

    Table table3("Pizza", "pizza.bin");
    table3.addAttributeInfo("id", 1);
    table3.addAttributeInfo("order_id", 2);
    table3.addAttributeInfo("sauce", 3);
    table3.addAttributeInfo("price", 4);

    Table table4("PizzaNew", "pizza_new.bin");
    table4.addAttributeInfo("id", 1);
    table4.addAttributeInfo("order_id", 2);
    table4.addAttributeInfo("price", 4);

    TableList tableList3(&type3);
    tableList3.addTable(table3);
    tableList3.addTable(table4);

    list<TableList*> tableLists = {&tableList1, &tableList2, &tableList3};
    LOADT("Tdescr.bin", tableLists);

    list<TableList*> recoveredTableLists = RECOVERYT("Tdescr.bin");

    cout << "___________Info from Tdescr___________" << endl;
    for (const auto& tableList : recoveredTableLists) {
        cout << "RelationType " << tableList->type->name << ":" << endl;

        cout << "  Attributes:" << endl;
        for (const auto& attr : tableList->type->attributes) {
            cout << "    ID: " << attr.id
                 << ", Name: " << attr.name
                 << ", Type: " << static_cast<int>(attr.type)
                 << ", Size: " << attr.size
                 << ", PK: " << (attr.isPrimaryKey)
                 << ", FK: " << (attr.isForeignKey)
                 << endl;
        }

        for (const auto& table : tableList->tables) {
            cout << "  Table Name: " << table.name << ", Storage File: " << table.storageFile << endl;
            for (const auto& attrInfo : table.attributeInfo) {
                cout << "    Attribute: " << attrInfo.first << ", ID: " << attrInfo.second << endl;
            }
        }
    }

    cout << endl;
    cout << "___________GetR test 1___________" << endl;
    GetR("Users", recoveredTableLists);

    cout << endl;
    cout << "___________GetR test 2___________" << endl;
    GetR("PizzaNew", recoveredTableLists);

    cout << endl;
    cout << "___________GetR test 3___________" << endl;
    GetR("NonExistentTable", recoveredTableLists);

    return 0;
}