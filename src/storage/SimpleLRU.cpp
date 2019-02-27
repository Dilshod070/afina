#include "SimpleLRU.h"

namespace Afina {
namespace Backend {

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Put(const std::string &key, const std::string &value)
{
	auto it = _lru_index.find(std::reference_wrapper<const std::string>(key));
	if (it != _lru_index.end()) {
		_delete(it->second.get());
	}
	std::unique_ptr<lru_node> new_node(new lru_node(key, value));
	_insert(new_node.get());
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value)
{
	auto it = _lru_index.find(std::reference_wrapper<const std::string>(key));
	if (it == _lru_index.end()) {
		return false;
	}
	_delete(it->second.get());
	std::unique_ptr<lru_node> new_node(new lru_node(key, value));
	_insert(new_node.get());

}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Set(const std::string &key, const std::string &value)
{
	auto it = _lru_index.find(std::reference_wrapper<const std::string>(key));
	if (it == _lru_index.end()) {
		return false;
	}
	if (!_delete(it->second.get())) {
		return false;
	}
	std::unique_ptr<lru_node> new_node(new lru_node(key, value));
	_insert(new_node.get());
	return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Delete(const std::string &key)
{
	auto it = _lru_index.find(std::reference_wrapper<const std::string>(key));
	if (it == _lru_index.end()) {
		return false;
	}
	return _delete(it->second.get());
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Get(const std::string &key, std::string &value)
{
	auto it = _lru_index.find(std::reference_wrapper<const std::string>(key));
	if (it == _lru_index.end()) {
		return false;
	}
	value = it->second.get().value;
	return _move_to_tail(it->second.get());
}

bool SimpleLRU::_move_to_tail(lru_node &node)
{
  	if (node.next == nullptr) { // Node is already the last
  		return true;
  	} else if (node.prev == nullptr){ // Node is first
		node.next->prev = node.prev;
		node.next.swap(_lru_head);
		node.next.swap(_lru_tail->next);
		node.prev = _lru_tail;
		_lru_tail = &node;
		return true;
  	} else { // Node is in center
  		node.next->prev = node.prev;
  		node.next.swap(node.prev->next);
  		node.next.swap(_lru_tail->next);
  		node.prev = _lru_tail;
  		_lru_tail = &node;
  		return true;
  	}
    return false;
}

bool SimpleLRU::_insert(lru_node *node)
{
	size_t size = node->key.size() + node->value.size();
	if (size > _max_size) {
		return false;
	}
	while (size > _free_size) {
		_delete_oldest();
	}
	_free_size -= size;
	if (_lru_tail == nullptr) {
		_lru_head.reset(node);
		node->next = nullptr;
		node->prev = nullptr;
		_lru_tail = node;
	} else {
		_lru_tail->next.reset(node);
		node->next = nullptr;
		node->prev = _lru_tail;
		_lru_tail = node;
	}
	return true;
}

bool SimpleLRU::_delete_oldest()
{
	size_t size = _lru_head->key.size() + _lru_head->value.size();
	if (_lru_head->next == nullptr) {
		_lru_head = nullptr;
		_lru_tail = nullptr;
	} else {
		_lru_head.swap(_lru_head->next);
		_lru_head->next->prev = nullptr;
	}
	_free_size += size;
	return true;
}

bool SimpleLRU::_delete(lru_node &node)
{
	size_t size = node.key.size() + node.value.size();
	if (node.prev == nullptr){ // Node is first
		return _delete_oldest();
	} else if (node.next == nullptr) { // Node is last
		node.prev->next.swap(node.next);
		_lru_tail = node.prev;
		_free_size += size;
  		return true;
  	} else { // Node is in center
  		node.prev->next.swap(node.next);
		node.next->prev = node.prev;
  		_free_size += size;
  		return true;
  	}
    return false;
}

} // namespace Backend
} // namespace Afina
