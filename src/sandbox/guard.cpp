/*
 * guard.cpp
 *
 *  Created on: Nov 29, 2011
 *      Author: victor
 */

#include <iostream>

using namespace std;

typedef int Int;

Int PageCtrCnt[5] = {0,0,0,0,0};
Int PageDtrCnt[5] = {0,0,0,0,0};



struct Root {};
struct Derived: Root {};
struct Derived1: Derived {};
struct Derived2: Derived {};

template <typename Page>
class PageGuard {
	Page* page_;
	int idx_;
public:

	typedef Page PageType;

	PageGuard(): page_(NULL), idx_(0) {
		PageCtrCnt[idx_]++;
	}

	template <typename PageT>
	PageGuard(PageT* page): page_(static_cast<Page*>(page)), idx_(1)
	{
		PageCtrCnt[idx_]++;
	}

	template <typename PageT>
	PageGuard(const PageGuard<PageT>& guard): page_(static_cast<Page*>(guard.page_)), idx_(guard.idx_)
	{
		PageCtrCnt[idx_]++;
	}

	template <typename PageT>
	PageGuard(PageGuard<PageT>&& guard): page_(static_cast<Page*>(guard.page_)), idx_(guard.idx_)
	{
		 guard.idx_ 	= -1;
		 guard.page_	= NULL;
	}

	~PageGuard()
	{
		if (idx_ != -1) PageDtrCnt[idx_]--;
	}

	template <typename PageT>
	operator const PageT* () const {
		return static_cast<const PageT*>(page_);
	}

	template <typename PageT>
	operator PageT* () {
		return static_cast<PageT*>(page_);
	}

	void operator=(Page* page)
	{
		page_ = page;
	}

	template <typename P>
	void operator=(const PageGuard<P>& guard)
	{
		page_ = static_cast<Page*>(guard.page_);
	}


	bool operator==(const Page* page) const
	{
		return page_ == page;
	}

	bool operator!=(const Page* page) const
	{
		return page_ != page;
	}

	bool operator==(const PageGuard<Page>& page) const
	{
		return page_ == page.page_;
	}

	bool operator!=(const PageGuard<Page>& page) const
	{
		return page_ != page->page_;
	}

	const Page* page() const {
		return page_;
	}

	Page* page() {
		return page_;
	}

	const Page*& ref() const {
		return page_;
	}

	Page*& ref() {
		return page_;
	}

	const Page* operator->() const {
		return page_;
	}

	Page* operator->() {
		return page_;
	}

	template <typename PageT> friend class PageGuard;
};


typedef PageGuard<Root> RootG;
typedef PageGuard<Derived> DerivedG;
typedef PageGuard<Derived1> Derived1G;
typedef PageGuard<Derived2> Derived2G;



//RootG derived2(&val);

RootG get0(Derived2& val) {
	return &val;
}


DerivedG get1(Derived2& val) {
	return get0(val);
}

Derived2G get2(Derived2& val) {
	return get1(val);
}

int main(void) {

	{
		Derived2 val;
		RootG r;

		Derived2G d = get2(val);

		r = &val;

		d = r;

		cout<<"Page: "<<d.page()<<" "<<&val<<endl;
	}

	Int CtrTotal = 0, DtrTotal = 0;
	for (Int c = 0; c < (Int)(sizeof(PageCtrCnt)/sizeof(Int)); c++)
	{
		cout<<c<<" "<<PageCtrCnt[c]<<" "<<PageDtrCnt[c]<<" "<<(PageCtrCnt[c] + PageDtrCnt[c])<<endl;
		CtrTotal += PageCtrCnt[c];
		DtrTotal += PageDtrCnt[c];
	}

	cout<<"Total: "<<CtrTotal<<" "<<DtrTotal<<" "<<(CtrTotal + DtrTotal)<<endl;
}
