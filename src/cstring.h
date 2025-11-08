#pragma once

class cstring {
protected:
	char *m_str;
	unsigned long m_len;
	unsigned long m_maxLen;
	void resize(unsigned long p_newsize) {
		//assume new never fails
		//size^2 >= p_newsize
		char *nArray = p_newsize>0 ? (new char[p_newsize+1]) : (char*)0;
		if (m_str) {
			unsigned long i;
			unsigned long smallerSize = p_newsize;
			if (m_len < smallerSize) smallerSize = m_len;
			for (i = 0; i < smallerSize; i++) {
				nArray[i] = m_str[i]; //copy byte for byte
			}
			if(nArray)
				nArray[i] = 0; //i maybe greater then p_newsize
			delete[] m_str;
			m_len = smallerSize;
		}
		else {
			m_len = 0;
		}
		m_maxLen = p_newsize;
		m_str = nArray;
	}
public:
	cstring() {
		m_str = 0; m_maxLen = 0; m_len = 0;
	}
	cstring(const char*s) :cstring() {
		SetString((const char*)s);
	}
	cstring(const char*s, unsigned long len) :cstring() {
		SetString(s, len);
	}
	cstring(cstring &s) {
		SetString(s.str());
	}
	~cstring() {
		if (m_str) delete[] m_str;
	}
	void Clear() {
		m_len = 0;
	}
	void Destroy() {
		m_maxLen = m_len = 0;
		if (m_str) delete[] m_str;
		m_str = 0;
	}
	const unsigned long Size() { return m_len; }
	void Crop() {
		if (m_len< m_maxLen) {
			resize(m_len);
		}
	}
	const char *str() { return m_str; }
	void SetString(const char *p_str, unsigned long len) {
		unsigned long i;
		if(len>m_maxLen)
			resize(len);
		m_len = len;
		for (i = 0; i<len; i++) {
			m_str[i] = p_str[i];
		}
		m_len = i;
		if(m_maxLen>0)
			m_str[i] = 0;
	}
	void operator = (cstring &s) {
		SetString((const char*)s.str());
	}
	void operator = (const char *s) {
		SetString(s);
	}
	void SetString(const char *s) {
		m_len = 0;
		if (s) {
			unsigned long i;
			for (i=0; s[i]; i++) {}
			if (i > m_maxLen) {
				resize(i); //will allocate i+1
			}
			for (i=0; s[i]; i++) {
				m_str[i] = s[i];
			}
			m_len = i;
			if(m_maxLen>0) //maxLen could be 0 and will cause problems with m_str[i]= data
				m_str[i] = 0;
			//else m_str should be null
		}
	}
	char &operator[](long i) {
		return m_str[i];
	}

	long Compare(cstring &s) {
		unsigned long i;
		for (i = 0; i < m_len && i < s.m_len; i++) {
			if (s.m_str[i] != m_str[i]) {
				return m_str[i] - s.m_str[i];
			}
		}
		return m_str[i] - s.m_str[i]; //one is null or thier the same
	}
	long Compare(const char* s) {
		unsigned long i;
		for (i = 0; i < m_len && s[i]; i++) {
			if (s[i] != m_str[i]) {
				return m_str[i] - s[i];
			}
		}
		return m_str[i] - s[i]; //one is null or thier the same
	}
	long Compare(const char* s, unsigned long p_len) {
		unsigned long i;
		for (i = 0; i < m_len && i<p_len; i++) {
			if (s[i] != m_str[i]) {
				return m_str[i] - s[i];
			}
		}
		if (m_len == p_len) {
			if (i == p_len) return 0; //there the same (we reached end of both strings)
			return m_str[i] - s[i];
		}
		if (i == m_len) return 0-s[i];
		return m_str[i] - 0;
	}

	long isEqual(const char* s) { return (Compare(s) == 0) ? 1 : 0; }
	long isEqual(const char* s,unsigned int p_len) { 
		if (p_len != m_len) return 0;
		unsigned long i;
		for (i = 0; i < m_len && i < p_len; i++) {
			if (s[i] != m_str[i]) {
				return 0;
			}
		}
		return 1;
	}

	void Concat(const char *s) {
		long i;
		for (i = 0; s[i]; i++) {}
		if (m_len + i> m_maxLen) {
			resize(i + m_len); //will copy
		}
		for (i = 0; s[i]; i++) {
			m_str[m_len++] = s[i];
		}
		if(m_maxLen>0)
			m_str[m_len] = 0;
	}
	void operator += (const char *s) {
		Concat(s);
	}
};
