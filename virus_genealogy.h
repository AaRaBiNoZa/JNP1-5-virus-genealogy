#ifndef VIRUS_GENEALOGY_H
#define VIRUS_GENEALOGY_H

#include <iterator>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

class VirusNotFound : public std::exception {
  public:
    const char *what() const noexcept override {
        return "VirusNotFound";
    }
};
class VirusAlreadyCreated : public std::exception {
  public:
    const char *what() const noexcept override {
        return "VirusAlreadyCreated";
    }
};
class TriedToRemoveStemVirus : public std::exception {
  public:
    const char *what() const noexcept override {
        return "TriedToRemoveStemVirus";
    }
};

template <typename Virus>
class VirusGenealogy {
  public:
    using virus_set_iterator = typename std::set<std::shared_ptr<Virus>>::iterator;

    struct Iterator {
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = Virus;
        using pointer = std::shared_ptr<Virus>;
        using reference = const Virus &;
        explicit Iterator(virus_set_iterator ptr) : m_ptr(ptr) {
        }
        Iterator() = default;

        reference operator*() const {
            return **m_ptr;
        }

        pointer operator->() {
            return *m_ptr;
        }

        // Prefix increment
        Iterator &operator++() {
            m_ptr++;
            return *this;
        }

        // Postfix increment
        Iterator operator++(int) {
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        // Prefix decrement
        Iterator &operator--() {
            m_ptr--;
            return *this;
        }

        // Postfix decrement
        Iterator operator--(int) {
            Iterator tmp = *this;
            --(*this);
            return tmp;
        }

        friend bool operator==(const Iterator &a, const Iterator &b) {
            return a.m_ptr == b.m_ptr;
        };
        friend bool operator!=(const Iterator &a, const Iterator &b) {
            return a.m_ptr != b.m_ptr;
        };

      private:
        virus_set_iterator m_ptr;
    };
    using children_iterator = Iterator;

    VirusGenealogy(const VirusGenealogy<Virus> &) = delete;
    VirusGenealogy &operator=(const VirusGenealogy<Virus> &) = delete;

    // Tworzy nową genealogię.
    // Tworzy także węzeł wirusa macierzystego o identyfikatorze stem_id.
    explicit VirusGenealogy(typename Virus::id_type const &stem_id)
        : stem_id(stem_id), graph{{stem_id, Node(stem_id)}} {
    }

    // Zwraca identyfikator wirusa macierzystego.
    typename Virus::id_type get_stem_id() const noexcept {
        return stem_id;
    }

    // Zwraca iterator pozwalający przeglądać listę identyfikatorów
    // bezpośrednich następników wirusa o podanym identyfikatorze.
    // Zgłasza wyjątek VirusNotFound, jeśli dany wirus nie istnieje.
    // Iterator musi spełniać koncept bidirectional_iterator oraz
    // typeid(*v.get_children_begin()) == typeid(const Virus &).
    VirusGenealogy<Virus>::children_iterator
    get_children_begin(typename Virus::id_type const &id) const {
        auto it = graph.find(id);
        if (it == graph.end()) {
            throw VirusNotFound();
        }

        return Iterator(it->second.children_virus_ptrs.begin());
    }

    // Iterator wskazujący na element za końcem wyżej wspomnianej listy.
    // Zgłasza wyjątek VirusNotFound, jeśli dany wirus nie istnieje.
    VirusGenealogy<Virus>::children_iterator
    get_children_end(typename Virus::id_type const &id) const {
        auto it = graph.find(id);
        if (it == graph.end()) {
            throw VirusNotFound();
        }

        return Iterator(it->second.children_virus_ptrs.end());
    }

    // Zwraca listę identyfikatorów bezpośrednich poprzedników wirusa
    // o podanym identyfikatorze.
    // Zgłasza wyjątek VirusNotFound, jeśli dany wirus nie istnieje.
    std::vector<typename Virus::id_type> get_parents(typename Virus::id_type const &id) const {
        auto it = graph.find(id);
        if (it == graph.end()) {
            throw VirusNotFound();
        }

        return std::vector<typename Virus::id_type>(it->second.parent_ids.begin(),
                                                    it->second.parent_ids.end());
    }

    // Sprawdza, czy wirus o podanym identyfikatorze istnieje.
    bool exists(typename Virus::id_type const &id) const noexcept {
        return graph.find(id) != graph.end();
    }

    // Zwraca referencję do obiektu reprezentującego wirus o podanym
    // identyfikatorze.
    // Zgłasza wyjątek VirusNotFound, jeśli żądany wirus nie istnieje.
    const Virus &operator[](typename Virus::id_type const &id) const {
        auto it = graph.find(id);
        if (it == graph.end()) {
            throw VirusNotFound();
        }

        return *(it->second.virus);
    }

    // Tworzy węzeł reprezentujący nowy wirus o identyfikatorze id
    // powstały z wirusów o podanych identyfikatorach parent_ids.
    // Zgłasza wyjątek VirusAlreadyCreated, jeśli wirus o identyfikatorze
    // id już istnieje.
    // Zgłasza wyjątek VirusNotFound, jeśli któryś z wyspecyfikowanych
    // poprzedników nie istnieje.
    void create(typename Virus::id_type const &id,
                std::vector<typename Virus::id_type> const &parent_ids) {
        if (parent_ids.empty()) {
            return;
        }
        auto it_new_node = graph.find(id);
        if (it_new_node != graph.end()) {
            throw VirusAlreadyCreated();
        }

        std::vector<parent_id_iterator> parents_its;

        for (auto parent_id : parent_ids) {
            auto temporary = graph.find(parent_id);
            if (temporary == graph.end()) {
                throw VirusNotFound();
            }
            parents_its.push_back(temporary);
        }

        auto new_node = Node(id);
        std::vector<virus_set_iterator> edges_to_parents;

        for (auto parent_id : parent_ids) {
            new_node.parent_ids.insert(parent_id);
        }

        try {
            for (auto parent_it : parents_its) {
                auto [it, added] = parent_it->second.children_virus_ptrs.insert(new_node.virus);
                edges_to_parents.push_back(it);
            }
            graph.insert({id, new_node});
        } catch (std::exception &e) {
            for (size_t i = 0; i < edges_to_parents.size(); ++i) {
                parents_its[i]->second.children_virus_ptrs.erase(edges_to_parents[i]);
            }
            throw;
        }
    }

    // Tworzy węzeł reprezentujący nowy wirus o identyfikatorze id
    // powstały z wirusa o podanym identyfikatorze parent_id.
    // Zgłasza wyjątek VirusAlreadyCreated, jeśli wirus o identyfikatorze
    // id już istnieje.
    // Zgłasza wyjątek VirusNotFound, jeśli któryś z wyspecyfikowanych
    // poprzedników nie istnieje.
    void create(typename Virus::id_type const &id, typename Virus::id_type const &parent_id) {
        create(id, std::vector<typename Virus::id_type>(1, parent_id));
    }

    // Dodaje nową krawędź w grafie genealogii.
    // Zgłasza wyjątek VirusNotFound, jeśli któryś z podanych wirusów nie istnieje.
    void connect(typename Virus::id_type const &child_id,
                 typename Virus::id_type const &parent_id) {
        auto child_it = graph.find(child_id);
        auto parent_it = graph.find(parent_id);
        if (child_it == graph.end() || parent_it == graph.end()) {
            throw VirusNotFound();
        }

        if (!child_it->second.parent_ids.insert(parent_id).second) {
            return;
        }

        try {
            parent_it->second.children_virus_ptrs.insert(child_it->second.virus);
        } catch (std::exception &e) {
            child_it->second.parent_ids.erase(parent_id);
            throw;
        }
    }

    // Usuwa wirus o podanym identyfikatorze.
    // Zgłasza wyjątek VirusNotFound, jeśli żądany wirus nie istnieje.
    // Zgłasza wyjątek TriedToRemoveStemVirus przy próbie usunięcia
    // wirusa macierzystego.
    void remove(typename Virus::id_type const &id) {
        if (id == stem_id) {
            throw TriedToRemoveStemVirus();
        }
        auto it = graph.find(id);
        if (it == graph.end()) {
            throw VirusNotFound();
        }

        std::vector<parent_id_iterator> parent_its;

        for (auto parent_id : it->second.parent_ids) {
            parent_its.push_back(graph.find(parent_id));
        }

        for (auto child : it->second.children_virus_ptrs) {
            auto child_node = graph.find(child->get_id());
            child_node->second.parent_ids.erase(id);
            if (child_node->second.parent_ids.empty()) {
                try {
                    remove(child_node->first);
                } catch (std::exception &e) {
                    child_node->second.parent_ids.insert(id);
                    throw;
                }
            }
        }

        for (auto parent_it : parent_its) {
            parent_it->second.children_virus_ptrs.erase(it->second.virus);
        }
        graph.erase(id);
    }

  private:
    class Node {
      public:
        std::shared_ptr<Virus> virus;
        std::set<typename Virus::id_type> parent_ids;
        std::set<std::shared_ptr<Virus>> children_virus_ptrs;

        explicit Node(typename Virus::id_type const &virus_id) {
            virus = std::make_shared<Virus>(virus_id);
        }
    };

    typename Virus::id_type const stem_id;
    std::map<typename Virus::id_type, Node> graph{};
    using parent_id_iterator = typename std::map<typename Virus::id_type, Node>::iterator;
};

#endif // VIRUS_GENEALOGY_H
