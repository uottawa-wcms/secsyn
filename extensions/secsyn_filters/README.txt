Each syndication profile can define filters - these are, essentially, form
elements.

The secsyn filters module allows the user to create a filter set - they can pick a
profile type, then add instances of the filters to the filter set.

Then, these filters can be used to get a list of objects that match the filters.
These objects can be used in multiple ways by other modules.

As an example, autosyndication is done by creating a filter set and then specifying
how often and where you want to syndicate all the objects that match that
filter set.

CAUTION: When building this, the term "profile" was used for filter set. This
should be cleaned up, but for now be very careful when reading the word "profile".
Sometimes it is used to mean a Syndication Profile and sometimes it means an
object from the secsyn_filters_profile table (which is a filter set).