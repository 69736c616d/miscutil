template <class T, size_t m_size>
class GenericRingBuffer
{
public :

    GenericRingBuffer()
    {
        m_write.store(0, std::memory_order::memory_order_relaxed);
        m_read.store(0, std::memory_order::memory_order_relaxed);
    }

    void push(T val)
    {
        while(!try_push(val));
    }

    void pop(T *val)
    {
        while(!try_pop(val));
    }

    void pop_bulk(T *dst, size_t len)
    {
        while(!try_pop_bulk(dst, len));

    }

    void push_bulk(T *src, size_t len)
    {
        while(!try_push_bulk(src, len));
    }

private:

    bool try_push(T val)
    {
        const auto currentWHead = m_write.load(std::memory_order_relaxed);
        const auto nextWHead = currentWHead + 1;
        if (currentWHead - m_read.load(std::memory_order_acquire) <= m_size - 1) {
            m_buffer[currentWHead % m_size] = val;
            m_write.store(nextWHead, std::memory_order_release);
            return true;
        }

        return false;
    }

    bool try_pop(T *pval)
    {
        const auto currentRHead = m_read.load(std::memory_order_relaxed);
        const auto nextRHead = currentRHead + 1;

        if (currentRHead == m_write.load(std::memory_order_acquire)) {
            return false;
        }

        *pval = m_buffer[currentRHead % m_size];
        m_read.store(nextRHead, std::memory_order_release);

        return true;
    }


    bool try_pop_bulk(T *dst, size_t len)
    {
        const auto currentRHead = m_read.load(std::memory_order_relaxed);
        const auto currentWHead = m_write.load(std::memory_order_acquire);

        if (currentWHead >= currentRHead) {
            if (currentWHead -  currentRHead < len)
                return false;
        }

        else if (currentWHead < currentRHead){
            if ((m_size - currentRHead) + currentWHead < len)
                return false;
        }

        size_t nextRHead = (currentRHead + len) % m_size;
        if (nextRHead < currentRHead) {
            memcpy(dst, m_buffer + currentRHead, (m_size - currentRHead) * sizeof(T));
            memcpy(dst + (m_size - currentRHead), m_buffer, nextRHead * sizeof(T));

        }

        else {
            memcpy(dst, m_buffer + currentRHead, len);
        }

        m_read.store(nextRHead, std::memory_order_release);

        return true;
    }

    bool try_push_bulk(T *src, size_t len)
    {
        const auto currentWHead = m_write.load(std::memory_order_relaxed);
        const auto currentRHead = m_read.load(std::memory_order_acquire);

        if (currentWHead > currentRHead) {
            if ((m_size - currentWHead) + currentRHead < len)
                return false;
        }

        else if (currentWHead < currentRHead) {
            if (currentRHead -  currentWHead < len)
                return false;
        }

        const auto nextWHead = (currentWHead + len) % m_size;
        if (nextWHead < currentWHead) {
            memcpy(m_buffer + currentWHead, src, (m_size - currentWHead) * sizeof(T));

            memcpy(m_buffer, src + (m_size - currentWHead),
                   nextWHead);
        }

        else
            memcpy(m_buffer + currentWHead, src, len);

        m_write.store(nextWHead, std::memory_order_release);
        return true;
    }

private :
    char cachePad_1[64];
    std::atomic<size_t> m_write;
    char cachePad_2[64];
    std::atomic<size_t> m_read;
    T m_buffer[m_size];
};
