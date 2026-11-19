#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <memory>
#include <algorithm>
#include <ctime>
#include <limits>

using namespace std;

struct Book {
    int id;
    string title, author, category;
    int totalCopies, availableCopies;
};

class Member {
protected:
    int id;
    string name, phone;

public:
    Member(int i, string n, string p) : id(i), name(n), phone(p) {}
    virtual ~Member() = default;

    int getId() const { return id; }
    string getName() const { return name; }
    string getPhone() const { return phone; }

    virtual int borrowingLimit() const = 0;
    virtual string getType() const = 0;

    void display() const {
        cout << left << setw(10) << id << setw(24) << name
             << setw(18) << phone << getType() << '\n';
    }
};

class Student : public Member {
public:
    Student(int i, string n, string p) : Member(i, n, p) {}
    int borrowingLimit() const override { return 3; }
    string getType() const override { return "Student"; }
};

class Faculty : public Member {
public:
    Faculty(int i, string n, string p) : Member(i, n, p) {}
    int borrowingLimit() const override { return 5; }
    string getType() const override { return "Faculty"; }
};

struct Transaction {
    int id;
    int bookId;
    int memberId;
    string issueDate;
    string dueDate;
    string returnDate;
    double fine;
    bool returned;
    bool finePaid;
};

class Library {
private:
    vector<Book> books;
    vector<unique_ptr<Member>> members;
    vector<Transaction> transactions;
    int nextTransactionId = 1;

    const string bookFile = "books.txt";
    const string memberFile = "members.txt";
    const string transactionFile = "transactions.txt";

    string cleanField(const string& value) const {
        string result = value;
        replace(result.begin(), result.end(), '|', '/');
        replace(result.begin(), result.end(), '\n', ' ');
        replace(result.begin(), result.end(), '\r', ' ');
        return result;
    }

    string currentDate() const {
        time_t now = time(nullptr);
        tm* local = localtime(&now);
        stringstream ss;
        ss << put_time(local, "%Y-%m-%d");
        return ss.str();
    }

    time_t dateToTime(const string& date) const {
        tm t = {};
        istringstream ss(date);
        ss >> get_time(&t, "%Y-%m-%d");
        if (ss.fail()) return static_cast<time_t>(-1);
        t.tm_hour = 12;
        t.tm_min = 0;
        t.tm_sec = 0;
        t.tm_isdst = -1;
        return mktime(&t);
    }

    int daysBetween(const string& first, const string& second) const {
        time_t a = dateToTime(first);
        time_t b = dateToTime(second);
        if (a == static_cast<time_t>(-1) || b == static_cast<time_t>(-1))
            return 0;
        return static_cast<int>(difftime(b, a) / (60 * 60 * 24));
    }

    string addDays(const string& date, int days) const {
        time_t value = dateToTime(date);
        if (value == static_cast<time_t>(-1)) return date;
        value += static_cast<time_t>(days) * 24 * 60 * 60;
        tm* local = localtime(&value);
        stringstream ss;
        ss << put_time(local, "%Y-%m-%d");
        return ss.str();
    }

    int readInt(const string& prompt) const {
        int value;
        while (true) {
            cout << prompt;
            if (cin >> value) {
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                return value;
            }
            cout << "Please enter a valid number.\n";
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }
    }

    string readText(const string& prompt) const {
        string value;
        do {
            cout << prompt;
            getline(cin, value);
            if (value.empty()) cout << "This field cannot be empty.\n";
        } while (value.empty());
        return cleanField(value);
    }

    Book* findBook(int bookId) {
        for (auto& book : books)
            if (book.id == bookId) return &book;
        return nullptr;
    }

    const Book* findBook(int bookId) const {
        for (const auto& book : books)
            if (book.id == bookId) return &book;
        return nullptr;
    }

    Member* findMember(int memberId) {
        for (auto& member : members)
            if (member->getId() == memberId) return member.get();
        return nullptr;
    }

    const Member* findMember(int memberId) const {
        for (const auto& member : members)
            if (member->getId() == memberId) return member.get();
        return nullptr;
    }

    int activeBooksForMember(int memberId) const {
        int count = 0;
        for (const auto& t : transactions)
            if (t.memberId == memberId && !t.returned) count++;
        return count;
    }

    void saveBooks() const {
        ofstream out(bookFile);
        for (const auto& b : books)
            out << b.id << '|' << b.title << '|' << b.author << '|'
                << b.category << '|' << b.totalCopies << '|'
                << b.availableCopies << '\n';
    }

    void saveMembers() const {
        ofstream out(memberFile);
        for (const auto& m : members)
            out << m->getId() << '|' << m->getName() << '|'
                << m->getPhone() << '|' << m->getType() << '\n';
    }

    void saveTransactions() const {
        ofstream out(transactionFile);
        for (const auto& t : transactions)
            out << t.id << '|' << t.bookId << '|' << t.memberId << '|'
                << t.issueDate << '|' << t.dueDate << '|'
                << t.returnDate << '|' << fixed << setprecision(2) << t.fine
                << '|' << t.returned << '|' << t.finePaid << '\n';
    }

    void saveAll() const {
        saveBooks();
        saveMembers();
        saveTransactions();
    }

    void loadBooks() {
        ifstream in(bookFile);
        string line;
        while (getline(in, line)) {
            stringstream ss(line);
            Book b;
            string part;
            try {
                getline(ss, part, '|'); b.id = stoi(part);
                getline(ss, b.title, '|');
                getline(ss, b.author, '|');
                getline(ss, b.category, '|');
                getline(ss, part, '|'); b.totalCopies = stoi(part);
                getline(ss, part, '|'); b.availableCopies = stoi(part);
                books.push_back(b);
            } catch (...) {}
        }
    }

    void loadMembers() {
        ifstream in(memberFile);
        string line;
        while (getline(in, line)) {
            stringstream ss(line);
            string part, name, phone, type;
            try {
                getline(ss, part, '|'); int id = stoi(part);
                getline(ss, name, '|');
                getline(ss, phone, '|');
                getline(ss, type, '|');
                if (type == "Student")
                    members.push_back(make_unique<Student>(id, name, phone));
                else if (type == "Faculty")
                    members.push_back(make_unique<Faculty>(id, name, phone));
            } catch (...) {}
        }
    }

    void loadTransactions() {
        ifstream in(transactionFile);
        string line;
        while (getline(in, line)) {
            stringstream ss(line);
            Transaction t;
            string part;
            try {
                getline(ss, part, '|'); t.id = stoi(part);
                getline(ss, part, '|'); t.bookId = stoi(part);
                getline(ss, part, '|'); t.memberId = stoi(part);
                getline(ss, t.issueDate, '|');
                getline(ss, t.dueDate, '|');
                getline(ss, t.returnDate, '|');
                getline(ss, part, '|'); t.fine = stod(part);
                getline(ss, part, '|'); t.returned = (part == "1");
                getline(ss, part, '|'); t.finePaid = (part == "1");
                transactions.push_back(t);
                nextTransactionId = max(nextTransactionId, t.id + 1);
            } catch (...) {}
        }
    }

    void displayBook(const Book& b) const {
        cout << left << setw(8) << b.id << setw(25) << b.title
             << setw(22) << b.author << setw(18) << b.category
             << setw(10) << b.totalCopies << b.availableCopies << '\n';
    }

    void addBook() {
        int id = readInt("Enter Book ID: ");
        if (findBook(id)) {
            cout << "A book with this ID already exists.\n";
            return;
        }
        string title = readText("Enter title: ");
        string author = readText("Enter author: ");
        string category = readText("Enter category: ");
        int copies = readInt("Enter number of copies: ");
        if (copies <= 0) {
            cout << "Number of copies must be greater than zero.\n";
            return;
        }
        books.push_back({id, title, author, category, copies, copies});
        saveBooks();
        cout << "Book added successfully.\n";
    }

    void displayBooks() const {
        if (books.empty()) {
            cout << "No books are available.\n";
            return;
        }
        cout << left << setw(8) << "ID" << setw(25) << "Title"
             << setw(22) << "Author" << setw(18) << "Category"
             << setw(10) << "Total" << "Available\n";
        for (const auto& b : books) displayBook(b);
    }

    void searchBook() const {
        cout << "1. Search by ID\n2. Search by title\n3. Search by author\n";
        int choice = readInt("Choose: ");
        if (choice == 1) {
            int id = readInt("Enter Book ID: ");
            const Book* b = findBook(id);
            if (b) displayBook(*b);
            else cout << "Book not found.\n";
        } else if (choice == 2 || choice == 3) {
            string key = readText(choice == 2 ? "Enter title: " : "Enter author: ");
            bool found = false;
            for (const auto& b : books) {
                string value = choice == 2 ? b.title : b.author;
                string a = value, k = key;
                transform(a.begin(), a.end(), a.begin(), ::tolower);
                transform(k.begin(), k.end(), k.begin(), ::tolower);
                if (a.find(k) != string::npos) {
                    displayBook(b);
                    found = true;
                }
            }
            if (!found) cout << "No matching book found.\n";
        } else cout << "Invalid choice.\n";
    }

    void updateBook() {
        int id = readInt("Enter Book ID to update: ");
        Book* b = findBook(id);
        if (!b) {
            cout << "Book not found.\n";
            return;
        }
        cout << "Enter new details:\n";
        b->title = readText("Title: ");
        b->author = readText("Author: ");
        b->category = readText("Category: ");
        int newTotal = readInt("Total copies: ");
        int issued = b->totalCopies - b->availableCopies;
        if (newTotal < issued || newTotal < 1) {
            cout << "Total copies cannot be less than currently issued copies.\n";
            return;
        }
        b->totalCopies = newTotal;
        b->availableCopies = newTotal - issued;
        saveBooks();
        cout << "Book updated successfully.\n";
    }

    void deleteBook() {
        int id = readInt("Enter Book ID to delete: ");
        Book* b = findBook(id);
        if (!b) {
            cout << "Book not found.\n";
            return;
        }
        if (b->availableCopies != b->totalCopies) {
            cout << "Cannot delete a book while copies are issued.\n";
            return;
        }
        for (const auto& t : transactions) {
            if (t.bookId == id) {
                cout << "Cannot delete this book because it has transaction history.\n";
                return;
            }
        }
        books.erase(remove_if(books.begin(), books.end(),
                    [id](const Book& book) { return book.id == id; }), books.end());
        saveBooks();
        cout << "Book deleted successfully.\n";
    }

    void bookMenu() {
        int choice;
        do {
            cout << "\n--- BOOK MANAGEMENT ---\n"
                 << "1. Add book\n2. Display all books\n3. Search book\n"
                 << "4. Update book\n5. Delete book\n0. Back\n";
            choice = readInt("Enter choice: ");
            switch (choice) {
                case 1: addBook(); break;
                case 2: displayBooks(); break;
                case 3: searchBook(); break;
                case 4: updateBook(); break;
                case 5: deleteBook(); break;
                case 0: break;
                default: cout << "Invalid choice.\n";
            }
        } while (choice != 0);
    }

    void registerMember() {
        int id = readInt("Enter Member ID: ");
        if (findMember(id)) {
            cout << "A member with this ID already exists.\n";
            return;
        }
        string name = readText("Enter name: ");
        string phone = readText("Enter phone number: ");
        cout << "1. Student\n2. Faculty\n";
        int type = readInt("Select member type: ");
        if (type == 1)
            members.push_back(make_unique<Student>(id, name, phone));
        else if (type == 2)
            members.push_back(make_unique<Faculty>(id, name, phone));
        else {
            cout << "Invalid member type.\n";
            return;
        }
        saveMembers();
        cout << "Member registered successfully.\n";
    }

    void displayMembers() const {
        if (members.empty()) {
            cout << "No members registered.\n";
            return;
        }
        cout << left << setw(10) << "ID" << setw(24) << "Name"
             << setw(18) << "Phone" << "Type\n";
        for (const auto& m : members) m->display();
    }

    void searchMember() const {
        int id = readInt("Enter Member ID: ");
        const Member* m = findMember(id);
        if (m) {
            cout << left << setw(10) << "ID" << setw(24) << "Name"
                 << setw(18) << "Phone" << "Type\n";
            m->display();
            cout << "Borrowing limit: " << m->borrowingLimit() << '\n';
        } else cout << "Member not found.\n";
    }

    void removeMember() {
        int id = readInt("Enter Member ID to remove: ");
        if (!findMember(id)) {
            cout << "Member not found.\n";
            return;
        }
        if (activeBooksForMember(id) > 0) {
            cout << "Cannot remove a member who has books issued.\n";
            return;
        }
        for (const auto& t : transactions) {
            if (t.memberId == id && t.fine > 0 && !t.finePaid) {
                cout << "Cannot remove a member with an unpaid fine.\n";
                return;
            }
        }
        members.erase(remove_if(members.begin(), members.end(),
            [id](const unique_ptr<Member>& m) { return m->getId() == id; }),
            members.end());
        saveMembers();
        cout << "Member removed successfully.\n";
    }

    void memberMenu() {
        int choice;
        do {
            cout << "\n--- MEMBER MANAGEMENT ---\n"
                 << "1. Register member\n2. Display members\n"
                 << "3. Search member\n4. Remove member\n0. Back\n";
            choice = readInt("Enter choice: ");
            switch (choice) {
                case 1: registerMember(); break;
                case 2: displayMembers(); break;
                case 3: searchMember(); break;
                case 4: removeMember(); break;
                case 0: break;
                default: cout << "Invalid choice.\n";
            }
        } while (choice != 0);
    }

    void issueBook() {
        int memberId = readInt("Enter Member ID: ");
        Member* m = findMember(memberId);
        if (!m) {
            cout << "Member not found.\n";
            return;
        }
        if (activeBooksForMember(memberId) >= m->borrowingLimit()) {
            cout << "This member has reached the borrowing limit of "
                 << m->borrowingLimit() << " books.\n";
            return;
        }
        int bookId = readInt("Enter Book ID: ");
        Book* b = findBook(bookId);
        if (!b) {
            cout << "Book not found.\n";
            return;
        }
        if (b->availableCopies <= 0) {
            cout << "No copies of this book are currently available.\n";
            return;
        }
        string today = currentDate();
        Transaction t;
        t.id = nextTransactionId++;
        t.bookId = bookId;
        t.memberId = memberId;
        t.issueDate = today;
        t.dueDate = addDays(today, 14);
        t.returnDate = "-";
        t.fine = 0;
        t.returned = false;
        t.finePaid = true;
        transactions.push_back(t);
        b->availableCopies--;
        saveAll();
        cout << "Book issued successfully.\n";
        cout << "Issue date: " << t.issueDate << "\nDue date: " << t.dueDate << '\n';
    }

    void returnBook() {
        int memberId = readInt("Enter Member ID: ");
        int bookId = readInt("Enter Book ID: ");
        Transaction* active = nullptr;
        for (auto& t : transactions) {
            if (t.memberId == memberId && t.bookId == bookId && !t.returned) {
                active = &t;
                break;
            }
        }
        if (!active) {
            cout << "No active issue record found for these details.\n";
            return;
        }
        string today = currentDate();
        int lateDays = max(0, daysBetween(active->dueDate, today));
        active->fine = lateDays * 5.0;
        active->returnDate = today;
        active->returned = true;
        active->finePaid = (active->fine == 0);

        Book* b = findBook(bookId);
        if (b) b->availableCopies++;

        saveAll();
        cout << "Book returned successfully.\n";
        cout << "Return date: " << today << "\nLate days: " << lateDays
             << "\nFine: Rs. " << fixed << setprecision(2) << active->fine << '\n';
        if (active->fine > 0)
            cout << "Please pay the fine through Fine Management.\n";
    }

    void fineMenu() {
        int memberId = readInt("Enter Member ID: ");
        if (!findMember(memberId)) {
            cout << "Member not found.\n";
            return;
        }
        bool found = false;
        double totalDue = 0;
        cout << "\n--- FINE DETAILS ---\n";
        cout << left << setw(12) << "Trans. ID" << setw(12) << "Book ID"
             << setw(14) << "Fine (Rs.)" << "Status\n";
        for (const auto& t : transactions) {
            if (t.memberId == memberId && t.fine > 0) {
                found = true;
                cout << left << setw(12) << t.id << setw(12) << t.bookId
                     << setw(14) << fixed << setprecision(2) << t.fine
                     << (t.finePaid ? "Paid" : "Unpaid") << '\n';
                if (!t.finePaid) totalDue += t.fine;
            }
        }
        if (!found) {
            cout << "No fines found for this member.\n";
            return;
        }
        cout << "Total unpaid fine: Rs. " << fixed << setprecision(2)
             << totalDue << '\n';
        if (totalDue <= 0) return;

        cout << "1. Pay all unpaid fines\n0. Back\n";
        int choice = readInt("Enter choice: ");
        if (choice == 1) {
            for (auto& t : transactions)
                if (t.memberId == memberId && t.fine > 0 && !t.finePaid)
                    t.finePaid = true;
            saveTransactions();
            cout << "Fine payment recorded successfully.\n";
        }
    }

    void recommendations() const {
        string category = readText("Enter a category you like: ");
        bool found = false;
        cout << "\nRecommended books:\n";
        for (const auto& b : books) {
            string a = b.category, k = category;
            transform(a.begin(), a.end(), a.begin(), ::tolower);
            transform(k.begin(), k.end(), k.begin(), ::tolower);
            if (a.find(k) != string::npos && b.availableCopies > 0) {
                displayBook(b);
                found = true;
            }
        }
        if (!found) cout << "No available books found in this category.\n";
    }

    void popularBooks() const {
        if (books.empty()) {
            cout << "No books available.\n";
            return;
        }
        vector<pair<int, const Book*>> ranking;
        for (const auto& b : books) {
            int count = 0;
            for (const auto& t : transactions)
                if (t.bookId == b.id) count++;
            ranking.push_back({count, &b});
        }
        sort(ranking.begin(), ranking.end(),
             [](const auto& a, const auto& b) { return a.first > b.first; });
        cout << left << setw(8) << "ID" << setw(25) << "Title"
             << setw(22) << "Author" << "Times issued\n";
        for (const auto& item : ranking) {
            if (item.first > 0)
                cout << left << setw(8) << item.second->id
                     << setw(25) << item.second->title
                     << setw(22) << item.second->author
                     << item.first << '\n';
        }
        if (transactions.empty()) cout << "No issue history available yet.\n";
    }

    void transactionHistory() const {
        if (transactions.empty()) {
            cout << "No transactions available.\n";
            return;
        }
        cout << left << setw(10) << "Trans ID" << setw(10) << "Book ID"
             << setw(12) << "Member ID" << setw(13) << "Issue Date"
             << setw(13) << "Due Date" << setw(13) << "Return Date"
             << setw(12) << "Fine" << "Status\n";
        for (const auto& t : transactions) {
            cout << left << setw(10) << t.id << setw(10) << t.bookId
                 << setw(12) << t.memberId << setw(13) << t.issueDate
                 << setw(13) << t.dueDate << setw(13) << t.returnDate
                 << setw(12) << fixed << setprecision(2) << t.fine
                 << (t.returned ? "Returned" : "Issued") << '\n';
        }
    }

public:
    Library() {
        loadBooks();
        loadMembers();
        loadTransactions();
    }

    void run() {
        int choice;
        do {
            cout << "\n====================================\n"
                 << "       LIBRARY MANAGEMENT SYSTEM\n"
                 << "====================================\n"
                 << "1. Book Management\n"
                 << "2. Member Management\n"
                 << "3. Issue Book\n"
                 << "4. Return Book\n"
                 << "5. Fine Management\n"
                 << "6. Book Recommendations\n"
                 << "7. View Transaction History\n"
                 << "8. View Popular Books\n"
                 << "9. Save Records\n"
                 << "0. Exit\n";
            choice = readInt("Enter your choice: ");

            try {
                switch (choice) {
                    case 1: bookMenu(); break;
                    case 2: memberMenu(); break;
                    case 3: issueBook(); break;
                    case 4: returnBook(); break;
                    case 5: fineMenu(); break;
                    case 6: recommendations(); break;
                    case 7: transactionHistory(); break;
                    case 8: popularBooks(); break;
                    case 9:
                        saveAll();
                        cout << "All records saved successfully.\n";
                        break;
                    case 0:
                        saveAll();
                        cout << "Records saved. Thank you for using the system.\n";
                        break;
                    default:
                        cout << "Invalid choice. Please try again.\n";
                }
            } catch (const exception& e) {
                cout << "An error occurred: " << e.what() << '\n';
            }
        } while (choice != 0);
    }
};

int main() {
    try {
        Library library;
        library.run();
    } catch (const exception& e) {
        cout << "Program could not start: " << e.what() << '\n';
        return 1;
    }
    return 0;
}