// This file Copyright © 2011-2022 Transmission authors and contributors.
// It may be used under the MIT (SPDX: MIT) license.
// License text can be found in the licenses/ folder.

#import <AppKit/AppKit.h>

typedef NSString* FilterType NS_TYPED_EXTENSIBLE_ENUM;

extern FilterType const FilterTypeNone;
extern FilterType const FilterTypeActive;
extern FilterType const FilterTypeDownload;
extern FilterType const FilterTypeSeed;
extern FilterType const FilterTypePause;
extern FilterType const FilterTypeError;

typedef NSString* FilterSearchType NS_TYPED_EXTENSIBLE_ENUM;

extern FilterSearchType const FilterSearchTypeName;
extern FilterSearchType const FilterSearchTypeTracker;

extern const NSInteger kGroupFilterAllTag;

@interface FilterBarController : NSViewController

@property(nonatomic, readonly) NSArray<NSString*>* searchStrings;

- (instancetype)init;

- (void)setFilter:(id)sender;
- (void)switchFilter:(BOOL)right;
- (void)setSearchText:(id)sender;
- (void)setSearchType:(id)sender;
- (void)setGroupFilter:(id)sender;
- (void)reset:(BOOL)updateUI;
- (void)focusSearchField;

- (void)setCountAll:(NSUInteger)all
             active:(NSUInteger)active
        downloading:(NSUInteger)downloading
            seeding:(NSUInteger)seeding
             paused:(NSUInteger)paused
              error:(NSUInteger)error;

@end
