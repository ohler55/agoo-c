// Copyright 2018 by Peter Ohler, All Rights Reserved

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <agoo.h>
#include <agoo/gqlintro.h>
#include <agoo/gqlvalue.h>
#include <agoo/graphql.h>
#include <agoo/log.h>
#include <agoo/page.h>
#include <agoo/res.h>
#include <agoo/sdl.h>
#include <agoo/server.h>

static agooText		emptyResp = NULL;

static void
empty_handler(agooReq req) {
    if (NULL == emptyResp) {
	emptyResp = agoo_respond(200, NULL, 0, NULL);
	agoo_text_ref(emptyResp);
    }
    agoo_res_message_push(req->res, emptyResp);
}

typedef struct _artist	*Artist;

typedef struct _date {
    int	year;
    int	month;
    int	day;
} *Date;

// type Song {
//   name: String!
//   artist: Artist
//   duration: Int
//   release: Date
//   likes: Int
// }
typedef struct _song {
    const char		*name;
    Artist		artist;
    int32_t		duration;
    atomic_int		likes;
    struct _date	release;
} *Song;

// type Artist {
//   name: String!
//   songs: [Song]
//   origin: [String]
// }
struct _artist {
    const char	*name;
    Song	songs;    // NULL name terminated array
    const char	**origin; // NULL terminated array
};

// type Query {
//   artist(name: String!): Artist
//   artists: [Artist]
// }
typedef struct _query {
    struct _artist	*artists;
} *Query;

typedef struct _mutation {
    Query	q;
} *Mutation;

static const char	*morningside[] = {"Morningside", "Auckland", "New Zealand", NULL};
static struct _song	fazerdaze_songs[] = {
    {.name = "Jennifer", .artist = NULL , .duration = 240, .release = {2017, 5, 5}},
    {.name = "Lucky Girl", .artist = NULL, .duration = 170, .release = {2017, 5, 5}},
    {.name = "Friends", .artist = NULL, .duration = 194, .release = {2017, 5, 5}},
    {.name = "Reel", .artist = NULL, .duration = 193, .release = {2015, 11, 2}},
    {.name = NULL },
};

static const char	*stockholm[] = {"Stockholm", "Sweden", NULL};
static struct _song	boys_songs[] = {
    {.name = "Down In The Basement", .artist = NULL, .duration = 216, .release = {2018, 9, 28}},
    {.name = "Frogstrap", .artist = NULL, .duration = 195, .release = {2018, 9, 28}},
    {.name = "Worms", .artist = NULL, .duration = 208, .release = {2018, 9, 28}},
    {.name = "Amphetanarchy", .artist = NULL, .duration = 346, .release = {2018, 9, 28}},
    {.name = NULL },
};

static struct _artist	artists[] = {
    {.name = "Fazerdaze", .origin = morningside, .songs = fazerdaze_songs},
    {.name = "Viagra Boys", .origin = stockholm, .songs = boys_songs},
    {.name = NULL },
};

static struct _query	data = {
    .artists = artists,
};

static struct _mutation	moo = {
    .q = &data,
};

///// C hookups

static struct _gqlCclass	artist_class;

static int
song_name(agooErr err, gqlDoc doc, gqlCobj obj, gqlField field, gqlSel sel, gqlValue result, int depth) {
    Song	s = (Song)obj->ptr;
    const char	*key = (NULL == sel->alias) ? sel->name : sel->alias;

    return gql_object_set(err, result, key, gql_string_create(err, s->name, -1));
}

static int
song_artist(agooErr err, gqlDoc doc, gqlCobj obj, gqlField field, gqlSel sel, gqlValue result, int depth) {
    Song	s = (Song)obj->ptr;
    const char	*key = (NULL == sel->alias) ? sel->name : sel->alias;

    if (NULL == s->artist) {
	return gql_object_set(err, result, key, gql_null_create(err));
    }
    struct _gqlCobj	child = { .clas = &artist_class };
    gqlValue		co;

    if (NULL == (co = gql_object_create(err)) ||
	AGOO_ERR_OK != gql_object_set(err, result, key, co)) {
	return err->code;
    }
    child.ptr = (void*)s->artist;

    return gql_eval_sels(err, doc, (gqlRef)&child, field, sel->sels, co, depth + 1);
}

static int
song_duration(agooErr err, gqlDoc doc, gqlCobj obj, gqlField field, gqlSel sel, gqlValue result, int depth) {
    Song	s = (Song)obj->ptr;
    const char	*key = (NULL == sel->alias) ? sel->name : sel->alias;

    return gql_object_set(err, result, key, gql_int_create(err, s->duration));
}

static int
song_likes(agooErr err, gqlDoc doc, gqlCobj obj, gqlField field, gqlSel sel, gqlValue result, int depth) {
    Song	s = (Song)obj->ptr;
    const char	*key = (NULL == sel->alias) ? sel->name : sel->alias;
    int32_t	likes = (int32_t)(int64_t)atomic_load(&s->likes);

    return gql_object_set(err, result, key, gql_int_create(err, likes));
}

static int
song_release(agooErr err, gqlDoc doc, gqlCobj obj, gqlField field, gqlSel sel, gqlValue result, int depth) {
    Song	s = (Song)obj->ptr;
    const char	*key = (NULL == sel->alias) ? sel->name : sel->alias;
    char	date[16];
    int		len = snprintf(date, sizeof(date), "%04d-%02d-%02d", s->release.year, s->release.month, s->release.day);

    return gql_object_set(err, result, key, gql_string_create(err, date, len));
}

static struct _gqlCmethod	song_methods[] = {
    { .key = "name",      .func = song_name },
    { .key = "artist",   .func = song_artist },
    { .key = "duration", .func = song_duration },
    { .key = "release",  .func = song_release },
    { .key = "likes",    .func = song_likes },
    { .key = NULL,       .func = NULL },
};

static struct _gqlCclass	song_class = {
    .name = "Song",
    .methods = song_methods,
};

static int
artist_name(agooErr err, gqlDoc doc, gqlCobj obj, gqlField field, gqlSel sel, gqlValue result, int depth) {
    Artist	a = (Artist)obj->ptr;
    const char	*key = (NULL == sel->alias) ? sel->name : sel->alias;

    return gql_object_set(err, result, key, gql_string_create(err, a->name, -1));
}

static int
artist_songs(agooErr err, gqlDoc doc, gqlCobj obj, gqlField field, gqlSel sel, gqlValue result, int depth) {
    Artist		a = (Artist)obj->ptr;
    const char		*key = (NULL == sel->alias) ? sel->name : sel->alias;
    gqlValue		list = gql_list_create(err, NULL);
    gqlValue		co;
    struct _gqlField	cf;
    struct _gqlCobj	child = { .clas = &song_class };
    int			d2 = depth + 1;

    if (NULL == list ||
	AGOO_ERR_OK != gql_object_set(err, result, key, list)) {
	return err->code;
    }
    memset(&cf, 0, sizeof(cf));
    cf.type = sel->type->base;

    for (Song s = a->songs; NULL != s->name; s++) {
	if (NULL == (co = gql_object_create(err)) ||
	    AGOO_ERR_OK != gql_list_append(err, list, co)) {
	    return err->code;
	}
	child.ptr = s;
	if (AGOO_ERR_OK != gql_eval_sels(err, doc, (gqlRef)&child, &cf, sel->sels, co, d2)) {
	    return err->code;
	}
    }
    return AGOO_ERR_OK;
}

static int
artist_origin(agooErr err, gqlDoc doc, gqlCobj obj, gqlField field, gqlSel sel, gqlValue result, int depth) {
    Artist	a = (Artist)obj->ptr;
    const char	*key = (NULL == sel->alias) ? sel->name : sel->alias;
    gqlValue	list = gql_list_create(err, NULL);
    gqlValue	val;

    for (const char **s = a->origin; NULL != *s; s++) {
	if (NULL == (val = gql_string_create(err, *s, -1)) ||
	    AGOO_ERR_OK != gql_list_append(err, list, val)) {
	    return err->code;
	}
    }
    return gql_object_set(err, result, key, list);
}

static struct _gqlCmethod	artist_methods[] = {
    { .key = "name",   .func = artist_name },
    { .key = "songs",  .func = artist_songs },
    { .key = "origin", .func = artist_origin },
    { .key = NULL,     .func = NULL },
};

static struct _gqlCclass	artist_class = {
    .name = "Artist",
    .methods = artist_methods,
};

static int
query_artist(agooErr err, gqlDoc doc, gqlCobj obj, gqlField field, gqlSel sel, gqlValue result, int depth) {
    Query	q = (Query)obj->ptr;
    const char	*key = (NULL == sel->alias) ? sel->name : sel->alias;
    gqlValue	nv = gql_extract_arg(err, field, sel, "name");
    const char	*name = NULL;

    if (NULL != nv) {
	name = gql_string_get(nv);
    }
    Artist	a = NULL;

    if (NULL != name) {
	for (a = q->artists; NULL != a; a++) {
	    if (0 == strcmp(a->name, name)) {
		break;
	    }
	}
    }
    if (NULL == a) {
	return gql_object_set(err, result, key, gql_null_create(err));
    }
    struct _gqlCobj	child = { .clas = &artist_class };
    gqlValue		co;

    if (NULL == (co = gql_object_create(err)) ||
	AGOO_ERR_OK != gql_object_set(err, result, key, co)) {
	return err->code;
    }
    child.ptr = (void*)a;

    return gql_eval_sels(err, doc, (gqlRef)&child, field, sel->sels, co, depth + 1);
}

static int
query_artists(agooErr err, gqlDoc doc, gqlCobj obj, gqlField field, gqlSel sel, gqlValue result, int depth) {
    Query		q = (Query)obj->ptr;
    const char		*key = (NULL == sel->alias) ? sel->name : sel->alias;
    gqlValue		list = gql_list_create(err, NULL);
    gqlValue		co;
    struct _gqlField	cf;
    struct _gqlCobj	child = { .clas = &artist_class };
    int			d2 = depth + 1;

    if (NULL == list ||
	AGOO_ERR_OK != gql_object_set(err, result, key, list)) {
	return err->code;
    }
    memset(&cf, 0, sizeof(cf));
    cf.type = sel->type->base;

    for (Artist a = q->artists; NULL != a->name; a++) {
	if (NULL == (co = gql_object_create(err)) ||
	    AGOO_ERR_OK != gql_list_append(err, list, co)) {
	    return err->code;
	}
	child.ptr = a;
	if (AGOO_ERR_OK != gql_eval_sels(err, doc, (gqlRef)&child, &cf, sel->sels, co, d2)) {
	    return err->code;
	}
    }
    return AGOO_ERR_OK;
}

static struct _gqlCmethod	query_methods[] = {
    { .key = "artist",  .func = query_artist },
    { .key = "artists", .func = query_artists },
    { .key = NULL,      .func = NULL },
};

static struct _gqlCclass	query_class = {
    .name = "query",
    .methods = query_methods,
};

static struct _gqlCobj	query_obj = {
    .clas = &query_class,
    .ptr = &data,
};


///// Mutation type setup

static int
mutation_like(agooErr err, gqlDoc doc, gqlCobj obj, gqlField field, gqlSel sel, gqlValue result, int depth) {
    Mutation	m = (Mutation)obj->ptr;
    const char	*key = sel->name;
    gqlValue	nv;
    const char	*artist = NULL;
    const char	*song = NULL;
    Song	found = NULL;

    if (NULL != (nv = gql_extract_arg(err, field, sel, "artist"))) {
	artist = gql_string_get(nv);
    }
    if (NULL != (nv = gql_extract_arg(err, field, sel, "song"))) {
	song = gql_string_get(nv);
    }
    for (Artist a = m->q->artists; NULL != a->name; a++) {
	if (0 == strcmp(a->name, artist)) {
	    for (found = a->songs; NULL != found->name; found++) {
		if (0 == strcmp(found->name, song)) {
		    atomic_fetch_add(&found->likes, 1);
		    break;
		}
	    }
	    break;
	}
    }
    if (NULL == found) {
	return gql_object_set(err, result, key, gql_null_create(err));
    }
    struct _gqlCobj	child = { .clas = &song_class, .ptr = (void*)found };
    gqlValue		co;

    if (NULL == (co = gql_object_create(err)) ||
	AGOO_ERR_OK != gql_object_set(err, result, key, co)) {
	return err->code;
    }
    return gql_eval_sels(err, doc, (gqlRef)&child, field, sel->sels, co, depth + 1);
}

static struct _gqlCmethod	mutation_methods[] = {
    { .key = "like", .func = mutation_like },
    { .key = NULL,     .func = NULL },
};

static struct _gqlCclass	mutation_class = {
    .name = "mutation",
    .methods = mutation_methods,
};

static struct _gqlCobj	mutation_obj = {
    .clas = &mutation_class,
    .ptr = &moo,
};

int
main(int argc, char **argv) {
    struct _agooErr	err = AGOO_ERR_INIT;
    int			port = 6464;

    // Setup the artist links in the song data.
    for (Artist a = data.artists; NULL != a->name; a++) {
	for (Song s = a->songs; NULL != s->name; s++) {
	    s->artist = a;
	}
    }
    if (AGOO_ERR_OK != agoo_init(&err, "songs") ||
	AGOO_ERR_OK != agoo_pages_set_root(&err, ".") ||
	AGOO_ERR_OK != agoo_bind_to_port(&err, port)) {
	printf("Failed to initialize, set root, or bind to port %d. %s\n", port, err.msg);
	return err.code;
    }
    if (AGOO_ERR_OK != agoo_load_graphql(&err, "/graphql", "song.graphql")) {
	return err.code;
    }
    agoo_query_object = &query_obj;
    agoo_mutation_object = &mutation_obj;

    // set up hooks or routes
    if (AGOO_ERR_OK != agoo_add_func_hook(&err, AGOO_GET, "/", empty_handler, true)) {
	return err.code;
    }
    agoo_server.thread_cnt = sysconf(_SC_NPROCESSORS_ONLN);

    // start the server and wait for it to be shutdown
    if (AGOO_ERR_OK != agoo_start(&err, AGOO_VERSION)) {
	printf("%s\n", err.msg);
	return err.code;
    }
    return 0;
}
