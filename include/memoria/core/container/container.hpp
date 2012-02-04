
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
#include <memoria/core/container/init.hpp>

#include <string>



#define MEMORIA_MODEL_METHOD_IS_NOT_IMPLEMENTED() throw MemoriaException(MEMORIA_SOURCE, std::string("Method is not implemented for " + String(me()->type_name())))

namespace memoria    {

template <typename Profile, typename SelectorType, typename ContainerTypeName> class CtrTF;

template <typename Name, typename Base, typename Types> class CtrPart;

template <typename Types> class Ctr;
template <typename Types> class Iter;

template <typename Allocator>
struct IParentCtrInterface
{
	typedef typename Allocator::CtrShared					CtrShared;
	typedef typename Allocator::Page::ID 			ID;

	virtual ID 	  GetRootID(void* caller, BigInt name)					= 0;
	virtual void  SetRootID(void* caller, BigInt name, const ID& root) 	= 0;

	virtual Allocator& GetAllocator()									= 0;
	virtual CtrShared* GetShared()										= 0;
};


template <typename TypesType>
class ContainerBase: public IParentCtrInterface<typename TypesType::Allocator> {
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
    typedef typename Allocator::CtrShared                                       CtrShared;
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
//    PageId                      root_;
    
protected:
    static ContainerMetadata*   reflection_;

public:
    ContainerBase()//: root_(0)
    {}

    ContainerBase(const ThisType& other)//: root_(other.root_)
    {}

    ContainerBase(ThisType&& other)//: root_(other.root_)
    {}



    void operator=(ThisType&& other) {
    	//this->root_ = other.root_;
    }

    void operator=(const ThisType& other) {
    	//this->root_ = other.root_;
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

    void set_root(const PageId &root)
    {
//        me()->allocator().SetRoot(me()->name(), root);
        me()->shared()->root_log() 	= root;
        me()->shared()->updated() 	= true;

        me()->SetRootID(this, me()->name(), root);
    }

    const PageId &root() const
    {
        const CtrShared* shared = me()->shared();

        if (shared->updated())
        {
        	return me()->shared()->root_log();
        }
        else {
        	return me()->shared()->root();
        }
    }

    const bool IsComposite() {
        return kCompositeContainer;
    }

    static ContainerMetadata * reflection() {
        return reflection_;
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

    typedef typename Types::Allocator                                           Allocator;
    typedef typename Types::Allocator::CtrShared                                CtrShared;
    typedef typename Types::Allocator::PageG									PageG;
    typedef typename PageG::Page::ID											ID;

    typedef typename Types::NodeBase                                           	NodeBase;
    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Types::Metadata                                        	Metadata;

public:

    typedef typename Types::ContainerTypeName									ContainerTypeName;
    typedef IParentCtrInterface<Allocator>										ParentCtrInterface;
    typedef ContainerTypeName                                                   Name;

private:

    Allocator&	allocator_;
    BigInt      name_;
    const char* model_type_name_;

    Logger logger_;
    static Logger class_logger_;

    CtrShared*	shared_;

    bool 		debug_;

    ParentCtrInterface* parent_ctr_;

public:


    Ctr(Allocator &allocator, BigInt name, bool create = false, const char* mname = NULL):
        Base(),
        allocator_(allocator),
        model_type_name_(mname != NULL ? mname : TypeNameFactory<ContainerTypeName>::cname()),
        logger_(model_type_name_, Logger::DERIVED, &allocator.logger()),
        debug_(false),
        parent_ctr_(NULL)
    {
    	if (create)
    	{
    		shared_ = allocator.GetCtrShared(name, true);

    		NodeBaseG node 		= me()->CreateNode(0, true, true);

    		allocator.SetRoot(name, node->id());

    		shared_->root_log() = node->id();
    		shared_->updated() 	= true;
    	}
    	else {
    		shared_ = allocator.GetCtrShared(name, false);
    	}

    	ref();
    }

    Ctr(ParentCtrInterface* parent, BigInt name, bool create, const char* mname):
        Base(),
        allocator_(parent->GetAllocator()),
        name_(name),
        model_type_name_(mname),
        logger_(model_type_name_, Logger::DERIVED, &allocator_.logger()),
        shared_(NULL),
        debug_(false),
        parent_ctr_(parent)
    {

    	if (create)
    	{
    		shared_ = parent->GetShared()->Get(name, create);

    		NodeBaseG node = me()->CreateNode(0, true, true);

    		shared_->root_log() = node->id();
    		shared_->updated() 	= true;

    		this->SetRootID(this, name, node->id());
    	}
    	else {
    		ID root_id = parent->GetRootID(this, name);

    		shared_ = parent->GetShared()->Get(name, true);

    		shared_->root() 	= root_id;
    		shared_->root_log() = 0;
    		shared_->updated() 	= false;
    	}

    	ref();
    }

    Ctr(Allocator &allocator, const ID& root_id, const char* mname = NULL):
            Base(),
            allocator_(allocator),
            name_(-1),
            model_type_name_(mname != NULL ? mname : TypeNameFactory<ContainerTypeName>::cname()),
            logger_(model_type_name_, Logger::DERIVED, &allocator.logger()),
            debug_(false),
            parent_ctr_(NULL)
    {
    	NodeBaseG root 	= allocator.GetPage(root_id, Allocator::READ);
    	Metadata  meta 	= me()->GetRootMetadata(root);
    	name_			= meta.model_name();
    	shared_ 		= allocator_.GetCtrShared(name_, false);

    	ref();
    }

    Ctr(ParentCtrInterface* parent, const ID& root_id, const char* mname):
    	Base(),
    	allocator_(parent->GetAllocator()),
    	name_(-1),
    	model_type_name_(mname),
    	logger_(model_type_name_, Logger::DERIVED, &allocator_.logger()),
    	debug_(false),
    	parent_ctr_(parent)
    {
    	NodeBaseG root 	= allocator_.GetPage(root_id, Allocator::READ);
    	Metadata  meta 	= me()->GetRootMetadata(root);
    	name_			= meta.model_name();
    	shared_ 		= parent->GetShared()->Get(name_, true);

    	ref();
    }

    Ctr(const MyType& other):
    	Base(other),
    	allocator_(other.allocator_),
    	model_type_name_(other.model_type_name_),
    	logger_(other.logger_),
    	shared_(other.shared_),
    	debug_(other.debug_),
    	parent_ctr_(NULL)
    {
    	ref();
    }

    Ctr(MyType&& other):
    	Base(std::move(other)),
    	allocator_(other.allocator_),
    	model_type_name_(other.model_type_name_),
    	logger_(other.logger_),
    	shared_(other.shared_),
    	debug_(other.debug_),
    	parent_ctr_(NULL)
    {
    	other.shared_ = NULL;
    }

    Ctr(MyType&& other, ParentCtrInterface* parent):
    	Base(std::move(other)),
    	allocator_(other.allocator_),
    	model_type_name_(other.model_type_name_),
    	logger_(other.logger_),
    	shared_(other.shared_),
    	debug_(other.debug_),
    	parent_ctr_(parent)
    {
    	other.shared_ = NULL;
    }

    ~Ctr() throw()
    {
    	unref();
    }

    virtual ID GetRootID(void* caller, BigInt name)
    {
    	if (caller == this)
    	{
    		if (parent_ctr_ == NULL)
    		{
    			return me()->allocator().GetRootID(name);
    		}
    		else {
    			return parent_ctr_->GetRootID(this, name);
    		}
    	}
    	else {
    		return get_child_root(name);
    	}
    }

    virtual void SetRootID(void* caller, BigInt name, const ID& root)
    {
    	if (caller == this)
    	{
    		if (parent_ctr_ == NULL)
    		{
    			me()->allocator().SetRoot(name, root);
    		}
    		else {
    			parent_ctr_->SetRootID(this, name, root);
    		}
    	}
    	else {
    		set_child_root(name, root);
    	}
    }

    virtual Allocator& GetAllocator() {
    	return allocator();
    }

    virtual CtrShared* GetShared() {
    	return shared_;
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

    BigInt name() const {
        return shared_->name();
    }

    BigInt& name() {
    	return shared_->name();
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

    	unref();
    	shared_				= other.shared_;
    	ref();

    	return *this;
    }

    MyType& operator=(MyType&& other)
    {
    	name_ 				= other.name_;
    	model_type_name_	= other.model_type_name_;
    	logger_				= other.logger_;
    	debug_				= other.debug_;
    	Base::operator=(std::move(other));

    	unref();
    	shared_				= other.shared_;

    	other.shared_		= NULL;

    	return *this;
    }

    const CtrShared* shared() const {
    	return shared_;
    }

    CtrShared* shared() {
    	return shared_;
    }

    void clear_shared() {
    	shared_ = NULL;
    }

private:
    void ref()
    {
    	if (shared_ != NULL)
    	{
    		shared_->ref();
    	}
    }

    void unref()
    {
    	if (shared_ != NULL && shared_->unref() == 0)
    	{
    		if (shared_->parent() == NULL)
    		{
    			allocator_.ReleaseCtrShared(shared_);
    		}
    		else {
    			shared_->parent()->RemoveChild(shared_);
    		}
    	}
    }

    void set_child_root(BigInt name, const ID& root_id)
    {
    	if (!root_id.is_null())
    	{
    		NodeBaseG root 	= allocator_.GetPage(me()->root(), Allocator::READ);
    		Metadata  meta 	= MyType::GetRootMetadata(root);
    		meta.roots(name) = root_id;
    		MyType::SetRootMetadata(root, meta);
    	}
    }

    ID get_child_root(BigInt name)
    {
    	if (!me()->root().is_null())
    	{
    		NodeBaseG root 	= allocator_.GetPage(me()->root(), Allocator::READ);
    		Metadata  meta 	= MyType::GetRootMetadata(root);
    		return meta.roots(name);
    	}
    	else {
    		return ID(0);
    	}
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
