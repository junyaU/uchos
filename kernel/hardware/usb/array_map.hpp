/**
 * @file array_map.hpp
 * @brief Fixed-size map implementation using array storage
 */

#pragma once

#include <array>
#include <optional>

namespace kernel::hw::usb
{
/**
 * @brief Fixed-size map container using array storage
 *
 * This template class provides a simple key-value map with a fixed
 * maximum capacity. It uses linear search for lookups, making it
 * suitable for small collections where the overhead of hash tables
 * or trees is not justified.
 *
 * @tparam K Key type
 * @tparam V Value type
 * @tparam N Maximum number of entries (default: 16)
 */
template<class K, class V, size_t N = 16>
class array_map
{
public:
	/**
	 * @brief Get value associated with a key
	 *
	 * Performs a linear search through the array to find the key.
	 *
	 * @param key Key to search for
	 * @return std::optional<V> Value if found, std::nullopt otherwise
	 *
	 * @note Time complexity: O(N)
	 */
	std::optional<V> get(const K& key) const
	{
		for (int i = 0; i < table_.size(); i++) {
			if (auto opt_k = table_[i].first; opt_k && opt_k.value() == key) {
				return table_[i].second;
			}
		}

		return std::nullopt;
	}

	/**
	 * @brief Insert or update a key-value pair
	 *
	 * Finds the first empty slot and inserts the key-value pair.
	 * Does not check for duplicate keys.
	 *
	 * @param key Key to insert
	 * @param value Value to associate with the key
	 *
	 * @note If the map is full, the insertion is silently ignored
	 * @note Time complexity: O(N)
	 */
	void put(const K& key, const V& value)
	{
		for (int i = 0; i < table_.size(); i++) {
			if (!table_[i].first) {
				table_[i].first = key;
				table_[i].second = value;
				return;
			}
		}
	}

	/**
	 * @brief Remove a key-value pair
	 *
	 * Searches for the key and removes it by setting the key to nullopt.
	 * The value is not cleared but becomes inaccessible.
	 *
	 * @param key Key to remove
	 *
	 * @note If the key is not found, no action is taken
	 * @note Time complexity: O(N)
	 */
	void remove(const K& key)
	{
		for (int i = 0; i < table_.size(); i++) {
			if (auto opt_k = table_[i].first; opt_k && opt_k.value() == key) {
				table_[i].first = std::nullopt;
				return;
			}
		}
	}

private:
	/**
	 * @brief Internal storage array
	 *
	 * Each entry is a pair of optional key and value. Empty slots
	 * have std::nullopt for the key.
	 */
	std::array<std::pair<std::optional<K>, V>, N> table_{};
};
} // namespace kernel::hw::usb
