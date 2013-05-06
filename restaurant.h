#ifndef _RESTAURANT_H_
#define _RESTAURANT_H_

#include <map>
#include <random>

#include "log_add.h"

using namespace std;

//
// Chinese Restaurant Process with customer and table tracking
//

typedef mt19937 RandomGenerator;

template <typename Dish, typename Comp = less<Dish> >
class Restaurant : protected map<Dish, int, Comp>
{
public:
    using map<Dish,int,Comp>::const_iterator;
    using map<Dish,int,Comp>::begin;
    using map<Dish,int,Comp>::end;

    Restaurant(double alpha=0, bool track_tables=false);
    Restaurant(double alpha, bool track_tables, Comp cmp);

    // seat & unseat customer; p0 only used when tracking table assignments and
    // the return value is the change in table count for that dish
    int increment(Dish d, double p0=1, RandomGenerator *rng=0);
    int increment(Dish d, bool sharing_table, RandomGenerator &rng);
    int decrement(Dish d, double p0=1, RandomGenerator *rng=0);
    
    void insert(Dish d, int count);

    // lookup functions
    int count(Dish d) const;
    int operator[](Dish d) const { return count(d); }
    double prob(Dish dish, double p0=1) const;
    double prob(Dish dish, double del_numer, double del_denom, double p0=1) const;
    int num_customers() const { return _total_customers; }
    int num_types() const { return map<Dish,int>::size(); }
    bool empty() const { return _total_customers == 0; }

    double log_prob(Dish dish, double log_p0=0) const;
    double log_prob(Dish dish, double del_numer, double del_denom, 
                    double log_p0=0) const; // nb. del_* are NOT logs

    // table-based functions: must be tracking tables to use these
    double prob_share_table(Dish dish) const;
    double prob_new_table(Dish dish, double p0=1) const;
    int num_tables(Dish dish) const;
    int num_tables() const;

    double alpha() const { return _alpha; }
    void set_alpha(double a) { _alpha = a; }

private:
    double _alpha;
    bool _track_tables;

    struct TableCounter
    {
        TableCounter() : tables(0) {};
        int tables;
        map<int, int> table_histogram; // num customers at table -> number tables
    };
    map<Dish, TableCounter, Comp> _dish_tables;
    int _total_customers, _total_tables;
};

template <typename Dish, typename Comp>
Restaurant<Dish,Comp>::Restaurant(double alpha, bool track_tables)
: _alpha(alpha), _track_tables(track_tables), 
  _total_customers(0), _total_tables(0)
{
}

template <typename Dish, typename Comp>
Restaurant<Dish,Comp>::Restaurant(double alpha, bool track_tables, Comp cmp)
: map<Dish, int, Comp>(cmp),
  _alpha(alpha), _track_tables(track_tables), 
  _total_customers(0), _total_tables(0)
{
}

template <typename Dish, typename Comp>
double 
Restaurant<Dish,Comp>::prob(Dish dish, double p0) const
{
    return (count(dish) + _alpha * p0) / (_total_customers + _alpha);
}

template <typename Dish, typename Comp>
double 
Restaurant<Dish,Comp>::prob(Dish dish, double del_numer, double del_denom, double p0)
const
{
    return (count(dish) + del_numer + _alpha * p0) 
         / (_total_customers + del_denom + _alpha);
}

template <typename Dish, typename Comp>
double 
Restaurant<Dish,Comp>::log_prob(Dish dish, double log_p0) const
{
    // FIXME: cache log(alpha)
    return Log<double>::add(log(count(dish)), log(_alpha) + log_p0) - log(_total_customers + _alpha);
}

template <typename Dish, typename Comp>
double 
Restaurant<Dish,Comp>::log_prob(Dish dish, double del_numer, double del_denom, double log_p0)
const
{
    // FIXME: cache log(alpha)
    return Log<double>::add(log(count(dish) + del_numer), log(_alpha) + log_p0) 
        - log(_total_customers + del_denom + _alpha);
}

template <typename Dish, typename Comp>
double 
Restaurant<Dish,Comp>::prob_share_table(Dish dish) const
{
    typename map<Dish, int>::const_iterator dcit = this->find(dish);
    if (dcit != end())
        return dcit->second / (dcit->second + _alpha);
    else
        return 0;
}

template <typename Dish, typename Comp>
double 
Restaurant<Dish,Comp>::prob_new_table(Dish dish, double p0) const
{
    typename map<Dish, int>::const_iterator dcit = this->find(dish);
    if (dcit != end())
        return _alpha * p0 / (dcit->second + _alpha);
    else
        return p0;
}

template <typename Dish, typename Comp>
int 
Restaurant<Dish,Comp>::increment(Dish dish, double p0, RandomGenerator *rng)
{
    assert(p0 > 0);

    int delta = 0;
    if (_track_tables)
    {
        assert(rng != 0);

        TableCounter &tc = _dish_tables[dish];

        //cout << "\nincrement for " << dish << " with p0 " << p0 << "\n";
        //cout << "\tBEFORE histogram: " << tc.table_histogram << " ";
        //cout << "count: " << count(dish) << " ";
        //cout << "tables: " << tc.tables << "\n";

        uniform_real_distribution<double> uniform_distribution(0, 1);

        // seated on a new or existing table?
        double pshare = prob_share_table(dish);
        double pnew = prob_new_table(dish, p0);
        //cout << "\t\tP0 " << p0 << " count(dish) " << count(dish)
            //<< " tables " << _dish_tables[dish]
            //<< " p(share) " << pshare << " p(new) " << pnew << "\n";
        double r = uniform_distribution(*rng) * (pshare + pnew);
        if (r < pnew) 
        {
            // assign to a new table
            tc.tables += 1;
            tc.table_histogram[1] += 1;
            _total_tables += 1;
            delta = 1;
        }
        else
        {
            // randomly assign to an existing table
            // remove constant denominator from inner loop
            r = uniform_distribution(*rng) * count(dish);
            for (map<int,int>::iterator
                 hit = tc.table_histogram.begin();
                 hit != tc.table_histogram.end();
                 ++hit)
            {
                r -= hit->first * hit->second;
                if (r <= 0)
                {
                    tc.table_histogram[hit->first+1] += 1;
                    hit->second -= 1;
                    if (hit->second == 0)
                        tc.table_histogram.erase(hit);
                    break;
                }
            }
            assert(r <= 0);
            delta = 0;
        }
    }

    map<Dish,int,Comp>::operator[](dish) += 1;
    _total_customers += 1;

    //if (_track_tables)
    //{
        //cout << "\tAFTER histogram: " << _dish_tables[dish].table_histogram << " ";
        //cout << "count: " << count(dish) << " ";
        //cout << "tables: " << _dish_tables[dish].tables << "\n";
    //}

    return delta;
}

template <typename Dish, typename Comp>
int 
Restaurant<Dish,Comp>::increment(Dish dish, bool sharing_table, RandomGenerator &rng)
{
    int delta = 0;
    if (_track_tables)
    {
        TableCounter &tc = _dish_tables[dish];

        if (!sharing_table)
        {
            // assign to a new table
            tc.tables += 1;
            tc.table_histogram[1] += 1;
            _total_tables += 1;
            delta = 1;
        }
        else
        {
            // randomly assign to an existing table
            // remove constant denominator from inner loop
            uniform_real_distribution<double> uniform_real(0, 1);
            double r = uniform_real(rng) * count(dish);
            for (map<int,int>::iterator
                 hit = tc.table_histogram.begin();
                 hit != tc.table_histogram.end();
                 ++hit)
            {
                r -= hit->first * hit->second;
                if (r <= 0)
                {
                    tc.table_histogram[hit->first+1] += 1;
                    hit->second -= 1;
                    if (hit->second == 0)
                        tc.table_histogram.erase(hit);
                    break;
                }
            }
            assert(r <= 0);
            delta = 0;
        }
    }

    map<Dish,int>::operator[](dish) += 1;
    _total_customers += 1;

    return delta;
}

template <typename Dish, typename Comp>
void 
Restaurant<Dish,Comp>::insert(Dish dish, int count)
{
    assert(!_track_tables);
    map<Dish,int>::operator[](dish) += 1;
    _total_customers += 1;
}

template <typename Dish, typename Comp>
int 
Restaurant<Dish,Comp>::count(Dish dish) const
{
    typename map<Dish, int>::const_iterator 
        dcit = this->find(dish);
    if (dcit != end())
        return dcit->second;
    else
        return 0;
}

template <typename Dish, typename Comp>
int 
Restaurant<Dish,Comp>::decrement(Dish dish, double p0, RandomGenerator *rng)
{
    typename map<Dish, int>::iterator dcit = this->find(dish);
    assert(dcit != end());

    int delta = 0;
    if (_track_tables)
    {
        assert(rng != 0);

        typename map<Dish, TableCounter>::iterator dtit = _dish_tables.find(dish);
        assert(dtit != _dish_tables.end());
        TableCounter &tc = dtit->second;

        //cout << "\ndecrement for " << dish << " with p0 " << p0 << "\n";
        //cout << "\tBEFORE histogram: " << tc.table_histogram << " ";
        //cout << "count: " << count(dish) << " ";
        //cout << "tables: " << tc.tables << "\n";

        uniform_real_distribution<double> uniform_distribution(0, 1);
        double r = uniform_distribution(*rng) * (*this)[dish];
        for (map<int,int>::iterator hit = tc.table_histogram.begin();
             hit != tc.table_histogram.end(); ++hit)
        {
            r -= hit->first * hit->second;
            if (r <= 0)
            {
                if (hit->first > 1)
                    tc.table_histogram[hit->first-1] += 1;
                else
                {
                    delta = -1;
                    tc.tables -= 1;
                    _total_tables -= 1;
                }

                hit->second -= 1;
                if (hit->second == 0) tc.table_histogram.erase(hit);
                break;
            }
        }

        assert(r <= 0);
    }

    // remove the customer
    dcit->second -= 1;
    _total_customers -= 1;
    assert(dcit->second >= 0);
    if (dcit->second == 0) erase(dcit);

    //if (_track_tables)
    //{
        //cout << "\tAFTER histogram: " << _dish_tables[dish].table_histogram << " ";
        //cout << "count: " << count(dish) << " ";
        //cout << "tables: " << _dish_tables[dish].tables << "\n";
    //}

    return delta;
}

template <typename Dish, typename Comp>
int 
Restaurant<Dish,Comp>::num_tables(Dish dish) const
{
    assert(_track_tables);
    typename map<Dish, TableCounter, Comp>::const_iterator 
        dtit = _dish_tables.find(dish);
    assert(dtit != _dish_tables.end());
    return dtit->second.tables;
}

template <typename Dish, typename Comp>
int 
Restaurant<Dish,Comp>::num_tables() const
{
    assert(_track_tables);
    return _total_tables;
}

#endif
