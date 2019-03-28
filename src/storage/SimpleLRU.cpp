#include "SimpleLRU.h"

namespace Afina {
namespace Backend {

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Put(const std::string &key, const std::string &value)
{
	auto it = _lru_index.find(key);
	if (it == _lru_index.end()) {
	    return _insert_kv(key, value);
	} else {
		return _update_kv(key, value);
	}
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value)
{
	auto it = _lru_index.find(key);
	if (it == _lru_index.end()) {
		return _insert_kv(key, value);
	} else {
		return false;
	}
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Set(const std::string &key, const std::string &value)
{
	auto it = _lru_index.find(key);
	if (it == _lru_index.end()) {
		return false;
	} else {
		return _update_kv(key, value);
	}
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Delete(const std::string &key)
{
	auto it = _lru_index.find(key);
	if (it == _lru_index.end()) {
		return false;
	}
	return _delete(it->second.get());
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Get(const std::string &key, std::string &value)
{
	auto it = _lru_index.find(key);
	if (it == _lru_index.end()) {
		return false;
	}
	value = it->second.get().value;
	return _move_to_tail(it->second.get());
}

bool SimpleLRU::_insert_kv(const std::string &key, const std::string &value)
{
	size_t size = key.size() + value.size();
	if (size > _max_size) {
		return false;
	}
	while (size > _free_size) {
		_delete_oldest();
	}
	lru_node *new_node(new lru_node(key, value));
	// std::unique_ptr<lru_node> new_node(new lru_node(key, value));
	_lru_index.insert(std::make_pair(std::reference_wrapper<const std::string>(new_node->key), 
									 std::reference_wrapper<lru_node>(*new_node)
									 // std::reference_wrapper<lru_node>(*new_node.get())
	));
	_free_size -= size;
	return _insert(*new_node);
	// return _insert(*new_node.get());
}

bool SimpleLRU::_update_kv(const std::string &key, const std::string &value)
{

	size_t size = key.size() + value.size();
	if (size > _max_size) {
		return false;
	}

	auto it = _lru_index.find(key);
	size_t prev_size = it->second.get().key.size() + it->second.get().value.size();
	_move_to_tail(it->second.get());
	while (size > _free_size + prev_size) {
		_delete_oldest();
	}
	_free_size += prev_size - size;
	
	it->second.get().value = value;
	return true;
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

bool SimpleLRU::_insert(lru_node &node)
{
	if (_lru_tail == nullptr) {
		node.next = nullptr;
		node.prev = nullptr;
		_lru_head.reset(&node);
		_lru_tail = &node;
	} else {
		node.next = nullptr;
		node.prev = _lru_tail;
		_lru_tail->next.reset(&node);
		_lru_tail = &node;
	}
	return true;
}

bool SimpleLRU::_delete(lru_node &node)
{
	if (node.prev == nullptr){ // Node is first
		return _delete_oldest();
	} else {
		size_t size = node.key.size() + node.value.size();
		_lru_index.erase(node.key);
		if (node.next == nullptr) { // Node is last
			node.prev->next.swap(node.next);
			_lru_tail = node.prev;
	  	} else { // Node is in center
	  		node.prev->next.swap(node.next);
			node.next->prev = node.prev;
	  	}
	  	_free_size += size;
	  	return true;
	}
}

bool SimpleLRU::_delete_oldest()
{
	size_t size = _lru_head->key.size() + _lru_head->value.size();
	_lru_index.erase(_lru_head->key);
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

} // namespace Backend
} // namespace Afina
