#ifndef PROTOCOL_H
#define PROTOCOL_H

//this class should not exist, it is just a worse std::variant, but unfortunately it is 2016

struct Protocol{
	virtual ~Protocol() = default;
};


#endif // PROTOCOL_H
