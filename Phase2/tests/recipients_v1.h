#ifndef RECIPIENTS_V1_H
#define RECIPIENTS_V1_H

#include "EntityId.h"
#include "Models.h"
#include <vector>

using namespace std;

// Builds the seven recipients R001 to R007 of Appendix A (recipients.csv).
inline vector<Recipient> buildrecipients()
{
    vector<Recipient> list;
    vector<FoodType> types;

    types.push_back(COOKED);
    types.push_back(PACKAGED);
    list.push_back(Recipient(EntityId("R", 1), "Hope Shelter", "SHELTER", 150, types,
                             true, true, 5));

    types.clear();
    types.push_back(PACKAGED);
    types.push_back(PRODUCE);
    types.push_back(BAKERY);
    list.push_back(Recipient(EntityId("R", 2), "City Food Bank", "FOOD_BANK", 300, types,
                             true, true, 6));

    types.clear();
    types.push_back(COOKED);
    list.push_back(Recipient(EntityId("R", 3), "Green Community Kitchen",
                             "COMMUNITY_KITCHEN", 120, types, true, true, 7));

    types.clear();
    types.push_back(COOKED);
    types.push_back(BAKERY);
    list.push_back(Recipient(EntityId("R", 4), "Sunrise Care Home", "CARE_HOME", 80, types,
                             true, true, 8));

    types.clear();
    types.push_back(COOKED);
    types.push_back(PACKAGED);
    list.push_back(Recipient(EntityId("R", 5), "Riverside School Meals", "SCHOOL", 200,
                             types, true, true, 9));

    types.clear();
    types.push_back(PACKAGED);
    types.push_back(PRODUCE);
    list.push_back(Recipient(EntityId("R", 6), "Bridge Pantry", "PANTRY", 100, types, true,
                             false, 10));

    types.clear();
    types.push_back(COOKED);
    list.push_back(Recipient(EntityId("R", 7), "Sevak Kitchen", "COMMUNITY_KITCHEN", 90,
                             types, false, true, 8));

    return list;
}

#endif