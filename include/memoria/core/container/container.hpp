
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_CONTAINER_MODEL_HPP
#define	_MEMORIA_CORE_CONTAINER_MODEL_HPP

#include <memoria/core/types/types.hpp>
#include <memoria/vapi/models/logs.hpp>
#include <memoria/core/types/typelist.hpp>
#include <memoria/core/tools/reflection.hpp>

#include <memoria/core/container/names.hpp>
#include <memoria/core/container/builder.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/dispatcher.hpp>
#include <memoria/core/container/defaults.hpp>
#include <memoria/core/container/macros.hpp>

#include <string>



#define MEMORIA_MODEL_METHOD_IS_NOT_IMPLEMENTED() throw MemoriaException(MEMORIA_SOURCE, std::string("Method is not implemented for " + String(me()->type_name())))

namespace memoria    {

template <typename Profile, typename SelectorType, typename ContainerTypeName> class CtrTF;

template <typename Name, typename Base, typename Types> class CtrPart;

template <typename Types> class Ctr;
template <typename Types> class Iter;


template <typename TypesType>
class ContainerBase: public TypesType::ContainerInterface {
public:

	typedef ContainerBase<TypesType>											ThisType;
	typedef Ctr<TypesType> 														MyType;

	typedef typename TypesType::ContainerTypeName                               ContainerTypeName;
    typedef ContainerTypeName                                                   Name;
    typedef TypesType                                                           Types;

    typedef typename Types::Allocator											Allocator;
    typedef typename Allocator::Transaction                                     Txn;
    typedef typename Allocator::Page                                            Page;
    typedef typename Allocator::PageG                                           PageG;
    typedef typename Allocator::Page::ID                                        PageId;
    typedef typename Types::NodeBase                                        	NodeBase;
    typedef typename Types::Metadata                                        	Metadata;


    typedef Iter<typename Types::IterTypes>										Iterator;

    template <typename Name>
    struct Containers {
        typedef typename Type2TypeMap<
                    Name,
                    typename Types::EmbeddedContainersList
        >::Result                                                               ContainersType;
    };

    static const bool kCompositeContainer = !ListSize<typename Types::EmbeddedContainersList>::Value;

private:
    PageId                      root_;
    
protected:
    static ContainerMetadata*   reflection_;

public:
    ContainerBase(): root_(0){}

    ContainerBase(const ThisType& other): root_(other.root_){}

    ContainerBase(ThisType&& other): root_(other.root_){}

    void operator=(ThisType&& other) {
    	this->root_ = other.root_;
    }

    void operator=(const ThisType& other) {
    	this->root_ = other.root_;
    }

    MyType* me() {
    	return static_cast<MyType*>(this);
    }

    const MyType* me() const {
    	return static_cast<const MyType*>(this);
    }

    BigInt IGetRawSize() {
        return -1;
    }

    static Int hash() {
        return reflection_->Hash();
    }

    void init_root(const PageId &root)
    {
    	PageG page = me()->allocator().GetPage(root);
        if (page == NULL) {
            throw NullPointerException(MEMORIA_SOURCE, "Requested page is not available");
        }

        if (page->model_hash() != me()->hash()) {
            throw MemoriaException(MEMORIA_SOURCE, "Invalid page type for this model");
        }

        root_ = root;

        Metadata meta = me()->GetRootMetadata(page); //(NodeBase*)
        me()->name() = meta.model_name();
    }

    void set_root(const PageId &root) {
        me()->allocator().SetRoot(me()->name(), root);
        root_ = root;
    }

    const PageId &root() const {
        return root_;
    }

    const bool IsComposite() {
        return kCompositeContainer;
    }

    static ContainerMetadata * reflection() {
        return reflection_;
    }

    virtual IDValue GetRootID() {
        if (!root_.is_null()) {
        	return IDValue(&root_);
        }
        else {
            return IDValue();
        }
    }
    
    // To be removed
    static Container* CreateContainer(const IDValue& rootID, memoria::vapi::ContainerCollection* container, BigInt name) {
    	return NULL;
    }

    static void Destroy() {
    	if (reflection_ != NULL)
    	{
    		cout<<"Delete model "<<reflection_<<endl;
    		delete reflection_;
    		reflection_ = NULL;
    	}
    }

    static Int Init(Int salt = 0) {
        if (reflection_ == NULL) {
            MetadataList list;
            Types::Pages::NodeDispatcher::BuildMetadataList(list);
            PageInitDispatcher<typename Types::DataPagesList>::BuildMetadataList(list);
            reflection_ = new ContainerMetadataImpl(TypeNameFactory<Name>::name(), list, Name::Code + salt, &CreateContainer);
        }
        return reflection_->Hash();
    }
};

template <typename TypesType>
ContainerMetadata* ContainerBase<TypesType>::reflection_ = NULL;



template <int Idx, typename Types>
class CtrHelper: public CtrPart<typename SelectByIndexTool<Idx, typename Types::List>::Result, CtrHelper<Idx - 1, Types>, Types> {
	typedef CtrHelper<Idx, Types> 								ThisType;
	typedef Ctr<Types> 											MyType;
	typedef CtrPart<typename SelectByIndexTool<Idx, typename Types::List>::Result, CtrHelper<Idx - 1, Types>, Types> Base;

public:
	CtrHelper(): Base() {}
	CtrHelper(const ThisType& other): Base(other) {}
	CtrHelper(ThisType&& other): Base(std::move(other)) {}

	void operator=(ThisType&& other) {
		Base::operator=(std::move(other));
	}

	void operator=(const ThisType& other) {
		Base::operator=(other);
	}
};

template <typename Types>
class CtrHelper<-1, Types>: public Types::template BaseFactory<Types>::Type {
	typedef CtrHelper<-1, Types> 								ThisType;
	typedef Ctr<Types> 											MyType;
	typedef typename Types::template BaseFactory<Types>::Type 	Base;

public:
	CtrHelper(): Base() {}
	CtrHelper(const ThisType& other): Base(other) {}
	CtrHelper(ThisType&& other): Base(std::move(other)) {}

	void operator=(ThisType&& other) {
		Base::operator=(std::move(other));
	}

	void operator=(const ThisType& other) {
		Base::operator=(other);
	}
};


template <typename Types>
class CtrStart: public CtrHelper<ListSize<typename Types::List>::Value - 1, Types> {

	typedef CtrStart<Types>			ThisType;
	typedef Ctr<Types> 				MyType;

	typedef CtrHelper<ListSize<typename Types::List>::Value - 1, Types> Base;
public:
	CtrStart(): Base() {}
	CtrStart(const ThisType& other): Base(other) {}
	CtrStart(ThisType&& other): Base(std::move(other)) {}

	void operator=(ThisType&& other) {
		Base::operator=(std::move(other));
	}

	void operator=(const ThisType& other) {
		Base::operator=(other);
	}
};




template <typename Types>
class Ctr: public CtrStart<Types> {
	typedef CtrStart<Types> 													Base;
public:
	typedef Ctr<Types>            												MyType;


    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::Page													Page;
    typedef typename Page::ID													ID;

public:

    typedef typename Types::ContainerTypeName									ContainerTypeName;
    typedef ContainerTypeName                                                   Name;

private:

    Allocator&	allocator_;
    BigInt      name_;
    const char* model_type_name_;

    Logger logger_;
    static Logger class_logger_;

    bool debug_;

public:

    typedef typename Base::NodeBase                                        		NodeBase;

    Ctr(Allocator &allocator, BigInt name, bool create = false, const char* mname = NULL):
        Base(),
        allocator_(allocator), name_(name),
        model_type_name_(mname != NULL ? mname : TypeNameFactory<ContainerTypeName>::cname()),
        logger_(model_type_name_, Logger::DERIVED, &class_logger_),
        debug_(false)
    {
    	if (create)
    	{
    		me()->create_new();
    	}
    	else {
    		ID root_id = me()->allocator().GetRootID(me()->name());
    		me()->init_root(root_id);
    	}
    }

    Ctr(Allocator &allocator, const ID& root_id, const char* mname = NULL):
            Base(),
            allocator_(allocator), name_(-1),
            model_type_name_(mname != NULL ? mname : TypeNameFactory<ContainerTypeName>::cname()),
            logger_(model_type_name_, Logger::DERIVED, &class_logger_),
            debug_(false)
    {
    	me()->init_root(root_id);
    }

    Ctr(const MyType& other): Base(other), allocator_(other.allocator_), name_(other.name_), model_type_name_(other.model_type_name_), logger_(other.logger_), debug_(other.debug_) {}
    Ctr(MyType&& other): Base(std::move(other)), allocator_(other.allocator_), name_(other.name_), model_type_name_(other.model_type_name_), logger_(other.logger_), debug_(other.debug_) {}


    virtual ~Ctr() throw() {
    }

    bool& debug() {
    	return debug_;
    }

    const bool& debug() const {
    	return debug_;
    }

    Allocator& allocator() {
        return allocator_;
    }

    Allocator& allocator() const {
    	return allocator_;
    }

    const char* type_name() const {
        return model_type_name_;
    }

    bool is_log(Int level)
    {
    	return logger_.IsLogEnabled(level);
    }

    memoria::vapi::Logger& logger() {
        return logger_;
    }

    static memoria::vapi::Logger& class_logger() {
    	return class_logger_;
    }

    const BigInt& name() const {
        return name_;
    }

    BigInt& name() {
    	return name_;
    }

    virtual BigInt ContainerName() {
        return name();
    }

    MyType* me()
    {
    	return this;
    }

    const MyType* me() const
    {
    	return this;
    }

    MyType& operator=(const MyType& other)
    {
    	name_ 				= other.name_;
    	model_type_name_	= other.model_type_name_;
    	logger_				= other.logger_;
    	debug_				= other.debug_;
    	Base::operator =(other);
    	return *this;
    }

    MyType& operator=(MyType&& other)
    {
    	name_ 				= other.name_;
    	model_type_name_	= other.model_type_name_;
    	logger_				= other.logger_;
    	debug_				= other.debug_;
    	Base::operator=(std::move(other));
    	return *this;
    }
};

template<
        typename Types
>
Logger Ctr<Types>::class_logger_(typeid(typename Types::ContainerTypeName).name(), Logger::DERIVED, &memoria::vapi::logger);




template <typename Types>  struct CtrTypesT: Types {

	typedef Types 						Base;
	typedef typename Types::CtrList 	List;

	template <typename Types_> struct BaseFactory {
		typedef typename Types::template CtrBaseFactory<Types_>::Type Type;
	};
};

template <typename Types>  struct IterTypesT: Types {

	typedef Types 						Base;
	typedef typename Types::IterList 	List;

	template <typename Types_> struct BaseFactory {
		typedef typename Types::template IterBaseFactory<Types_>::Type Type;
	};
};



}




#endif
