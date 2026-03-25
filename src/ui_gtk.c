/**
 * \file ui_gtk.c
 * \brief Interface graphique GTK du jeu Krojanty (plateau, affichage, réseau et IA locale).
 *
 * \details
 * - Ce fichier gère toute la fenêtre GTK : le dessin du plateau, la barre latérale (scores),
 *   les clics souris/clavier et les messages réseau.
 * - Il contient aussi l'intégration de l'IA locale (Minimax α–β via ai.c) avec un petit délai
 *   pour donner un effet “humain”, et un marqueur visuel de sélection quand l’IA joue.
 * - Le code est pensé “simple” : on explique chaque partie en mots faciles.
 *
 * \author (Étudiant) Votre Nom
 * \date 2025
 */

#include <gtk/gtk.h>
#include <pango/pangocairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdkkeysyms.h>
#include <glib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "ui_gtk.h"
#include "net.h"
#include "ai.h"

/* ============================================================
 * ===============   ÉTAT GLOBAL DE L’INTERFACE   =============
 * ============================================================
 */

/**
 * \struct App
 * \brief Regroupe tout l’état nécessaire pour la fenêtre.
 *
 * \details
 * On met tout ici pour éviter des variables globales partout.
 * - b, gs : plateau et état du jeu (points, captures, contrôle…)
 * - b0, gs0 : copie “départ” pour pouvoir recommencer facilement.
 * - has_sel/sel : gestion de la case sélectionnée (par humain ou IA).
 * - widgets GTK : fenêtre, zone de dessin, labels de la sidebar…
 * - net_state : infos réseau et thread de réception.
 * - IA : indicateurs (activée, occupée), config, délai d’attente.
 */
typedef struct {
    /* Jeu : état courant + sauvegarde du départ pour “Recommencer” */
    Board b, b0;
    GameState gs, gs0;
    GameResult last_result;

    /* Sélection visuelle d’une case (humain/IA) */
    bool  has_sel;
    Coord sel;

    /* Fenêtre et zone de dessin */
    GtkWidget *win;
    GtkWidget *area;

    /* Barre latérale (labels et bouton) */
    GtkWidget *sidebar;
    GtkWidget *lbl_game_title;
    GtkWidget *lbl_net;
    GtkWidget *lbl_title;
    GtkWidget *lbl_turn;
    GtkWidget *lbl_player;
    GtkWidget *lbl_pts_b;
    GtkWidget *lbl_pts_r;
    GtkWidget *lbl_cap_b;
    GtkWidget *lbl_cap_r;
    GtkWidget *lbl_kings;
    GtkWidget *lbl_sel;
    GtkWidget *btn_restart;

    /* Images (pièces et villes) */
    GdkPixbuf *px_bs, *px_rs;
    GdkPixbuf *px_bk, *px_rk;
    GdkPixbuf *px_city_b, *px_city_r;

    /* ======= Réseau ======= */
    NetworkState *net_state;       /**< NULL en local */
    pthread_t     network_thread;  /**< thread de réception */
    bool          network_thread_running;

    /* ======= IA locale ======= */
    int           ai_enabled;      /**< 1 si ce côté est piloté par IA */
    int           ai_busy;         /**< évite les re-entrées concurrentes */

    /* Effet “humain” : petit délai entre les coups IA */
    guint         ai_timer_id;     /**< 0 = aucun timer planifié */
    int           ai_delay_ms;     /**< délai ms entre les coups de l’IA */

    /* --- IA en thread de fond (anti-freeze UI) --- */
    GThread      *ai_thread;
    int           ai_thread_live;
} App;

/* ============================================================
 * ===============   OUTILS LABEL (THREAD-SAFE)   =============
 * ============================================================
 */

/**
 * \brief Petit paquet à envoyer au thread GTK pour mettre à jour un label.
 *
 * \details
 * Comme le thread réseau ne peut pas toucher l’UI directement,
 * on passe par g_idle_add() pour exécuter sur le thread GTK.
 */
typedef struct {
    GtkWidget *label;
    char *text;
    int   make_visible; /* -1: ignore, 0: hide, 1: show */
} LabelUpdate;

/**
 * \brief Applique une mise à jour de label depuis la file d’événements GTK.
 * \param data pointeur vers LabelUpdate alloué avec g_malloc/g_new
 * \return G_SOURCE_REMOVE pour ne pas réexécuter.
 */
static gboolean apply_label_update_cb(gpointer data){
    LabelUpdate *u = (LabelUpdate*)data;
    if (u->label && GTK_IS_LABEL(u->label)) {
        if (u->text) gtk_label_set_text(GTK_LABEL(u->label), u->text);
        if (u->make_visible == 1) gtk_widget_set_visible(u->label, TRUE);
        else if (u->make_visible == 0) gtk_widget_set_visible(u->label, FALSE);
    }
    g_free(u->text);
    g_free(u);
    return G_SOURCE_REMOVE;
}

/**
 * \brief Demande la mise à jour d’un label (thread-safe).
 * \param label le GtkLabel à modifier
 * \param text le nouveau texte (copié)
 * \param make_visible -1 = ne rien changer, 0 = cacher, 1 = montrer
 */
static void post_label_update(GtkWidget *label, const char *text, int make_visible){
    LabelUpdate *u = g_malloc0(sizeof *u);
    u->label = label;
    u->text  = g_strdup(text ? text : "");
    u->make_visible = make_visible;
    g_idle_add(apply_label_update_cb, u);
}

/* ============================================================
 * ====================   ASSETS (IMAGES)   ===================
 * ============================================================
 */

/** \brief Charge une image et renvoie NULL si échec (sans bloquer). */
static GdkPixbuf* load_px_or_null(const char* path){
    GError* err=NULL;
    GdkPixbuf* p = gdk_pixbuf_new_from_file(path, &err);
    if(!p && err){ g_error_free(err); }
    return p;
}

/** \brief Charge toutes les images nécessaires des pièces et des villes. */
static void load_assets(App* app){
    app->px_bs     = load_px_or_null("assets/pionbleu.png");
    app->px_rs     = load_px_or_null("assets/pionrouge.png");
    app->px_bk     = load_px_or_null("assets/roibleu.png");
    app->px_rk     = load_px_or_null("assets/roirouge.png");
    app->px_city_b = load_px_or_null("assets/citebleu.png");
    app->px_city_r = load_px_or_null("assets/citerouge.png");
}

/** \brief Libère proprement toutes les images (pixbufs). */
static void free_assets(App* app){
    if(app->px_bs)     g_object_unref(app->px_bs);
    if(app->px_rs)     g_object_unref(app->px_rs);
    if(app->px_bk)     g_object_unref(app->px_bk);
    if(app->px_rk)     g_object_unref(app->px_rk);
    if(app->px_city_b) g_object_unref(app->px_city_b);
    if(app->px_city_r) g_object_unref(app->px_city_r);
}

/* ============================================================
 * ================   BARRE LATÉRALE (SCORE)   ================
 * ============================================================
 */

/**
 * \brief Met à jour tous les labels de la barre latérale.
 *
 * \details
 * On rappelle ici les infos simples : tour courant, joueur,
 * points, captures, rois vivants, et la case sélectionnée.
 */
static void update_sidebar(App* app){
    const char* joueur = (app->b.current_player==PLAYER_BLUE ? "BLEU" : "ROUGE");

    char buf[128];
    gtk_label_set_text(GTK_LABEL(app->lbl_title), "STATISTIQUES");

    g_snprintf(buf,sizeof buf,"Tour : %d", app->b.turn_number);
    gtk_label_set_text(GTK_LABEL(app->lbl_turn), buf);

    g_snprintf(buf,sizeof buf,"Au joueur : %s", joueur);
    gtk_label_set_text(GTK_LABEL(app->lbl_player), buf);

    /* Encadré BLEU */
    g_snprintf(buf,sizeof buf,"Points bleu : %d", app->gs.blue_points);
    gtk_label_set_text(GTK_LABEL(app->lbl_pts_b), buf);
    g_snprintf(buf,sizeof buf,"Pions Capturés : %d", app->gs.blue_soldiers_captured);
    gtk_label_set_text(GTK_LABEL(app->lbl_cap_b), buf);

    /* Encadré ROUGE */
    g_snprintf(buf,sizeof buf,"Points rouge : %d", app->gs.red_points);
    gtk_label_set_text(GTK_LABEL(app->lbl_pts_r), buf);
    g_snprintf(buf,sizeof buf,"Pions Capturés : %d", app->gs.red_soldiers_captured);
    gtk_label_set_text(GTK_LABEL(app->lbl_cap_r), buf);

    /* Infos communes */
    g_snprintf(buf,sizeof buf,"Rois (B:%d, R:%d)", app->gs.blue_king_alive, app->gs.red_king_alive);
    gtk_label_set_text(GTK_LABEL(app->lbl_kings), buf);

    if(app->has_sel){
        char col = 'A' + app->sel.x;
        char row = '0' + (9 - app->sel.y);
        char sel[32]; g_snprintf(sel,sizeof sel,"Sélection : %c%c", col, row);
        gtk_label_set_text(GTK_LABEL(app->lbl_sel), sel);
    } else {
        gtk_label_set_text(GTK_LABEL(app->lbl_sel), "Sélection : —");
    }
}

/* ============================================================
 * ========================   STYLE CSS   =====================
 * ============================================================
 */

/** \brief Charge un peu de CSS pour embellir la barre latérale et la fenêtre victoire. */
static void load_sidebar_css(void){
    GtkCssProvider* css = gtk_css_provider_new();
    gtk_css_provider_load_from_string(css,
        ".sidebar { padding:12px; background-image: linear-gradient(135deg, #0055A4 0%, #FFFFFF 50%, #EF4135 100%); }"
        ".game-title { font-family: 'Cantarell','Noto Sans','DejaVu Sans',sans-serif; font-size:20pt; font-weight:900; letter-spacing:.5px; color:#0b1520; text-shadow:0 1px 0 rgba(255,255,255,.55); margin:2px 0 10px 0; }"
        ".net-badge { font-family:'Cantarell','Noto Sans','DejaVu Sans',sans-serif; font-size:11pt; font-weight:700; color:#0b1520; background:rgba(255,255,255,0.85); border:1px solid rgba(0,0,0,0.15); border-radius:8px; padding:6px 8px; margin:4px 0 10px 0; }"
        ".score-title { font-family:'Cantarell','Noto Sans','DejaVu Sans',sans-serif; font-size:16pt; font-weight:800; color:#0B3D91; margin:6px 0 8px 0; }"
        ".score-item { font-family:'Cantarell','Noto Sans','DejaVu Sans',sans-serif; font-size:12pt; font-weight:600; color:#1a2a3a; }"
        ".spaced { margin-top:4px; margin-bottom:6px; }"
        "button.big { font-weight:700; font-size:12pt; padding:6px 12px; color:#FFFFFF; background:#D7263D; border-radius:10px; }"
        "button.big:hover { background:#B81E32; }"
        ".thin-sep { min-height:1px; background:rgba(0,0,0,.15); margin:6px 0 10px 0; }"
        ".victory-root { background: radial-gradient(circle at 50% 40%, #0b1840 0%, #050a1e 60%, #020613 100%); padding:12px; }"
        ".victory-title { color:#ffffff; font-weight:900; font-size:22pt; text-shadow:0 2px 8px rgba(0,0,0,.6); margin:4px 0 8px 0; }"
        ".victory-sub { color:#dfe7ff; font-weight:600; font-size:12pt; margin:0 0 6px 0; }"

        /* === Encadrés BLEU/ROUGE === */
        ".team-title { font-weight:800; font-size:13pt; margin:6px 0 2px 0; }"
        ".team-panel { border-radius:10px; padding:8px 10px; margin:6px 0 10px 0;"
            "box-shadow: 0 2px 6px rgba(0,0,0,.25); }"
        ".team-blue  { background: linear-gradient(180deg, rgba(210,230,255,.95), rgba(180,210,255,.95));"
            "border:1px solid rgba(20,60,140,.35); }"
        ".team-red   { background: linear-gradient(180deg, rgba(255,220,220,.95), rgba(255,190,190,.95));"
            "border:1px solid rgba(160,40,40,.35); }"
    );
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(), GTK_STYLE_PROVIDER(css),
        GTK_STYLE_PROVIDER_PRIORITY_USER);
    g_object_unref(css);
}

/* ============================================================
 * ==============   FENÊTRE “VICTOIRE” (FEUX)   ===============
 * ============================================================
 * Juste pour le fun : petite fenêtre avec feux d’artifice.
 * C’est purement visuel, pour fêter la fin de partie.
 */

typedef struct { double x,y,vx,vy,life,maxlife,r,g,b; } FWParticle;

/** \brief Contexte interne de la fenêtre de victoire. */
typedef struct {
    GArray* particles;
    guint tick_id;
    guint timeout_close_id;
    GtkWidget* win;
    GtkWidget* area;
    gdouble w,h;
    guint last_ms;
} FireworksCtx;

/** \brief Ajoute un petit burst de particules à l’écran. */
static void fw_emit_burst(FireworksCtx* ctx, double cx, double cy){
    const int N = 80;
    for(int i=0;i<N;i++){
        double a = (2.0*G_PI)*((double)i/N) + g_random_double_range(-0.15,0.15);
        double speed = g_random_double_range(90.0, 200.0);
        FWParticle p;
        p.x=cx; p.y=cy; p.vx=cos(a)*speed; p.vy=sin(a)*speed - g_random_double_range(0,40);
        p.life=0; p.maxlife=g_random_double_range(1.5,2.2);
        p.r=g_random_double_range(0.6,1.0);
        p.g=g_random_double_range(0.6,1.0);
        p.b=g_random_double_range(0.6,1.0);
        g_array_append_val(ctx->particles, p);
    }
}

/** \brief “Tick” d’animation : fait bouger/éteindre les particules. */
static gboolean fw_tick(GtkWidget* widget, GdkFrameClock* clock, gpointer user_data){
    (void)widget;
    FireworksCtx* ctx = (FireworksCtx*)user_data;
    guint now = gdk_frame_clock_get_frame_time(clock)/1000;
    double dt = ctx->last_ms ? (now - ctx->last_ms)/1000.0 : 0.016;
    ctx->last_ms = now;

    for (guint i=0;i<ctx->particles->len;){
        FWParticle* p = &g_array_index(ctx->particles, FWParticle, i);
        p->vy += 120.0 * dt;
        p->x  += p->vx * dt;
        p->y  += p->vy * dt;
        p->life += dt;
        if(p->life >= p->maxlife) g_array_remove_index_fast(ctx->particles, i);
        else i++;
    }
    if (g_random_double() < 0.04){
        double cx = g_random_double_range(ctx->w*0.15, ctx->w*0.85);
        double cy = g_random_double_range(ctx->h*0.20, ctx->h*0.55);
        fw_emit_burst(ctx, cx, cy);
    }
    gtk_widget_queue_draw(ctx->area);
    return G_SOURCE_CONTINUE;
}

/** \brief Dessine les particules (queues + ronds), fond transparent. */
static void fw_draw(GtkDrawingArea *area, cairo_t *cr, int W, int H, gpointer user_data){
    (void)area;
    FireworksCtx* ctx = (FireworksCtx*)user_data;
    ctx->w = W; ctx->h = H;

    cairo_set_source_rgba(cr, 0,0,0,0);
    cairo_paint(cr);

    cairo_set_line_width(cr, 2.0);
    for (guint i=0;i<ctx->particles->len;i++){
        FWParticle* p = &g_array_index(ctx->particles, FWParticle, i);
        double t = p->life / p->maxlife;
        double alpha = (t<0.7? 1.0 : (1.0 - (t-0.7)/0.3));
        if(alpha<0) alpha=0;

        cairo_set_source_rgba(cr, p->r, p->g, p->b, alpha);
        cairo_move_to(cr, p->x, p->y);
        cairo_line_to(cr, p->x - p->vx*0.02, p->y - p->vy*0.02);
        cairo_stroke(cr);

        cairo_arc(cr, p->x, p->y, 2.5*(1.0-t), 0, 2*G_PI);
        cairo_fill(cr);
    }
}

/** \brief Ferme et nettoie la fenêtre de victoire. */
static void fw_close(FireworksCtx* ctx){
    if(ctx->tick_id) gtk_widget_remove_tick_callback(ctx->area, ctx->tick_id);
    if(ctx->timeout_close_id) g_source_remove(ctx->timeout_close_id);
    if(GTK_IS_WINDOW(ctx->win)) gtk_window_destroy(GTK_WINDOW(ctx->win));
    if(ctx->particles) g_array_free(ctx->particles, TRUE);
    g_free(ctx);
}
static gboolean fw_auto_close(gpointer data){ fw_close((FireworksCtx*)data); return G_SOURCE_REMOVE; }

/** \brief Ouvre la fenêtre “Victoire !” avec feux d’artifice. */
static void show_victory_window(GtkWindow* parent, const char* title_line, const char* sub_line){
    FireworksCtx* ctx = g_malloc0(sizeof *ctx);
    ctx->particles = g_array_new(FALSE, FALSE, sizeof(FWParticle));

    GtkWidget* win = gtk_window_new();
    ctx->win = win;
    gtk_window_set_transient_for(GTK_WINDOW(win), parent);
    gtk_window_set_modal(GTK_WINDOW(win), TRUE);
    gtk_window_set_title(GTK_WINDOW(win), "Victoire !");
    gtk_window_set_default_size(GTK_WINDOW(win), 520, 360);

    GtkWidget* root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_add_css_class(root, "victory-root");
    gtk_window_set_child(GTK_WINDOW(win), root);

    GtkWidget* title = gtk_label_new(title_line ? title_line : "Victoire !");
    gtk_widget_add_css_class(title, "victory-title");
    gtk_widget_set_halign(title, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(root), title);

    GtkWidget* sub = gtk_label_new(sub_line ? sub_line : "🎆🎆🎆");
    gtk_widget_add_css_class(sub, "victory-sub");
    gtk_widget_set_halign(sub, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(root), sub);

    ctx->area = gtk_drawing_area_new();
    gtk_widget_set_vexpand(ctx->area, TRUE);
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(ctx->area), fw_draw, ctx, NULL);
    gtk_box_append(GTK_BOX(root), ctx->area);

    GtkWidget* btn = gtk_button_new_with_label("OK");
    g_signal_connect_swapped(btn, "clicked", G_CALLBACK(fw_close), ctx);
    gtk_widget_set_halign(btn, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(root), btn);

    fw_emit_burst(ctx, 260, 150);
    ctx->tick_id = gtk_widget_add_tick_callback(ctx->area, fw_tick, ctx, NULL);
    ctx->timeout_close_id = g_timeout_add_seconds(4, fw_auto_close, ctx);

    gtk_window_present(GTK_WINDOW(win));
}

/** \brief Affiche la fenêtre de victoire selon le résultat courant. */
static void show_victory_dialog(App* app){
    if(app->last_result==GAME_ONGOING) return;
    const char* title =
        app->last_result==GAME_WIN_BLUE ? " 🎖️ Les BLEU ont gagné… sûrement un bug " :
        app->last_result==GAME_WIN_RED  ? " 🎖️ Victoire des ROUGE ! Même eux sont surpris " : "Match nul";
    show_victory_window(GTK_WINDOW(app->win), title, "🎆 Bravo ! 🎆");
}

/* ============================================================
 * ====================   DESSIN DU PLATEAU   =================
 * ============================================================
 */

/** \brief Dessine une image à l’échelle d’une case. */
static void draw_pixbuf_full(cairo_t* cr, GdkPixbuf* px, double X, double Y, double cell){
    if(!px) return;
    GdkPixbuf* scaled = gdk_pixbuf_scale_simple(px, (int)cell, (int)cell, GDK_INTERP_BILINEAR);
    if(!scaled) return;
    gdk_cairo_set_source_pixbuf(cr, scaled, X, Y);
    cairo_paint(cr);
    g_object_unref(scaled);
}

/** \brief Dessine le cadre de sélection (bleu/rouge) sur une case. */
static void draw_selection_marker(cairo_t* cr, double X, double Y, double cell, gboolean bleu){
    if(bleu) cairo_set_source_rgba(cr, 0.10, 0.35, 1.00, 0.12);
    else     cairo_set_source_rgba(cr, 1.00, 0.20, 0.25, 0.12);
    cairo_arc(cr, X + cell*0.5, Y + cell*0.5, cell*0.22, 0, 2*G_PI);
    cairo_fill(cr);
    if(bleu) cairo_set_source_rgb(cr, 0.12, 0.30, 0.95);
    else     cairo_set_source_rgb(cr, 0.90, 0.18, 0.18);
    cairo_set_line_width(cr, 1.5);
    cairo_rectangle(cr, X+3.0, Y+3.0, cell-6.0, cell-6.0);
    cairo_stroke(cr);
    cairo_set_line_width(cr, 1.0);
}

/**
 * \brief Dessine entièrement le plateau (cases, contrôles, pièces, sélection).
 * \details
 * - On calcule la taille des cases pour garder des marges et un plateau centré.
 * - On colore les zones contrôlées par Bleu/Rouge et la diagonale centrale.
 * - On dessine les pièces (images, sinon lettre).
 * - On affiche la case sélectionnée (humain ou IA).
 */
static void draw_board(GtkDrawingArea *area, cairo_t *cr, int W, int H, gpointer user_data){
    (void)area;
    App* app = (App*)user_data;
    const int N = BOARD_SIZE;

    const double M = 20.0;
    double maxW = (W - 2*M), maxH = (H - 2*M);
    if(maxW<100) maxW=100;
    if(maxH<100) maxH=100;
    double cell = ((maxW < maxH ? maxW : maxH) / (double)N);
    double offx = (W - cell*N)/2.0;
    double offy = (H - cell*N)/2.0;

    cairo_set_source_rgb(cr, 1,1,1);
    cairo_paint(cr);

    for (int y=0; y<N; ++y){
        for (int x=0; x<N; ++x){
            double X = offx + x*cell;
            double Y = offy + y*cell;

            /* fond */
            cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
            cairo_rectangle(cr, X, Y, cell, cell);
            cairo_fill(cr);

            /* zone de contrôle (couleur douce) */
            ControlOwner co = app->gs.control[y][x];
            if (co == CTRL_BLUE){
                cairo_set_source_rgba(cr, 0.812, 0.910, 1.000, 0.60);
                cairo_rectangle(cr, X, Y, cell, cell);
                cairo_fill(cr);
            } else if (co == CTRL_RED){
                cairo_set_source_rgba(cr, 1.000, 0.839, 0.839, 0.60);
                cairo_rectangle(cr, X, Y, cell, cell);
                cairo_fill(cr);
            }

            /* villes et diagonal “milieu” si neutre */
            bool isCityB = (x==CITY_BLUE.x && y==CITY_BLUE.y);
            bool isCityR = (x==CITY_RED.x  && y==CITY_RED.y);
            if(!isCityB && !isCityR && co==CTRL_NONE && (x + y == N - 1)){
                cairo_set_source_rgba(cr, 1.0, 0.92, 0.40, 0.55);
                cairo_rectangle(cr, X, Y, cell, cell);
                cairo_fill(cr);
            }

            /* pièce */
            Piece p = board_get(&app->b, (Coord){x,y});
            if (p != EMPTY){
                GdkPixbuf* px=NULL;
                if(p==BLUE_SOLDIER) px=app->px_bs;
                else if(p==RED_SOLDIER) px=app->px_rs;
                else if(p==BLUE_KING)   px=app->px_bk;
                else if(p==RED_KING)    px=app->px_rk;
                if(px) draw_pixbuf_full(cr, px, X, Y, cell);
                else {
                    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
                    cairo_set_font_size(cr, cell*0.6);
                    cairo_move_to(cr, X + cell*0.25, Y + cell*0.7);
                    cairo_show_text(cr, (p==BLUE_KING||p==RED_KING) ? "K" : "S");
                }
            }else{
                if(isCityB && app->px_city_b) draw_pixbuf_full(cr, app->px_city_b, X, Y, cell);
                if(isCityR && app->px_city_r) draw_pixbuf_full(cr, app->px_city_r, X, Y, cell);
            }

            /* sélection courante */
            if (app->has_sel && app->sel.x==x && app->sel.y==y){
                Piece psel = board_get(&app->b, app->sel);
                gboolean bleu = (psel==BLUE_SOLDIER || psel==BLUE_KING);
                draw_selection_marker(cr, X, Y, cell, bleu);
            }
        }
    }

    /* grille fine */
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
    cairo_set_source_rgb(cr, 0.20, 0.20, 0.20);
    cairo_set_line_width(cr, 1.0);
    for(int i=0;i<=N;i++){
        double x = offx + i*cell + 0.5;
        cairo_move_to(cr, x, offy+0.5);
        cairo_line_to(cr, x, offy + N*cell - 0.5);
    }
    for(int i=0;i<=N;i++){
        double y = offy + i*cell + 0.5;
        cairo_move_to(cr, offx+0.5, y);
        cairo_line_to(cr, offx + N*cell - 0.5, y);
    }
    cairo_stroke(cr);
}

/* ============================================================
 * ===============   HELPERS MOUVEMENTS <-> 4C   ==============
 * ============================================================
 * Conversion de coup en 4 octets (A1A3) pour le protocole
 * réseau minimaliste et simple.
 */

/** \brief Convertit un Move en 4 caractères “A1A3”. */
static void move_to_4chars(Move m, char out[4]) {
    out[0] = (char)('A' + m.from.x);
    out[1] = (char)('0' + (9 - m.from.y));
    out[2] = (char)('A' + m.to.x);
    out[3] = (char)('0' + (9 - m.to.y));
}

/** \brief Parse 4 caractères “A1A3” en Move (retourne 0 si invalide). */
static int move_from_4chars(const char in[4], Move* out) {
    if(in[0]<'A' || in[0]>'I') return 0;
    if(in[2]<'A' || in[2]>'I') return 0;
    if(in[1]<'1' || in[1]>'9') return 0;
    if(in[3]<'1' || in[3]>'9') return 0;
    char tmp[8];
    snprintf(tmp, sizeof tmp, "%c%c %c%c", in[0], in[1], in[2], in[3]);
    return move_from_str(tmp, out);
}

/* ============================================================
 * =====================   PARTIE IA LOCALE   =================
 * ============================================================
 * Idée : quand c’est au tour de l’IA locale, on attend un peu
 * (délai “humain”), on montre la case sélectionnée, puis on joue.
 */

/** \brief Donne si “moi” = Bleu (client) ou Rouge (serveur) en réseau. */
static Player my_player(const App* app){
    if(!app->net_state) return app->b.current_player; /* local */
    return (app->net_state->mode==GAME_MODE_CLIENT)? PLAYER_BLUE : PLAYER_RED;
}

/** \brief Envoie le coup au pair si on est en réseau. */
static void send_move_if_net(App* app, Move m){
    if (app->net_state && app->net_state->mode!=GAME_MODE_LOCAL && app->net_state->connected){
        char four[4]; move_to_4chars(m, four);
        net_send4(app->net_state->net_fd, four);
        app->net_state->is_my_turn = false;
    }
}

/**
 * \brief Dit si c’est bien à l’IA locale de jouer maintenant.
 * \details
 * - IA activée, partie en cours, autorisation réseau (notre tour),
 *   et joueur courant = “moi” (bleu si client, rouge si serveur).
 */
static gboolean is_local_ai_turn(const App* app){
    if(!app->ai_enabled) return FALSE;
    if(app->last_result != GAME_ONGOING) return FALSE;
    if(app->net_state && app->net_state->mode!=GAME_MODE_LOCAL && !app->net_state->is_my_turn)
        return FALSE;
    if (app->b.current_player != my_player(app)) return FALSE;
    return TRUE;
}

/* ---- Déclaration (utilisée par le réseau et l’UI) ---- */
static void schedule_ai_if_needed(App* app);

/** \brief Contexte pour appliquer le coup IA après avoir affiché la sélection. */
typedef struct { App* app; Move m; } AiMoveCtx;

/* ---------- Worker IA en thread de fond ---------- */
typedef struct {
    Board      b_snapshot;
    GameState  gs_snapshot;
    Player     me_snapshot;
    App       *app;
} AiWorkerArgs;

static gboolean apply_ai_move_cb(gpointer data); /* forward */

/* Exécute ai_choose_move hors du thread GTK, puis poste apply_ai_move_cb */
static gpointer ai_worker_func(gpointer data){
    AiWorkerArgs *args = (AiWorkerArgs*)data;
    App *app = args->app;

    Move best = (Move){{0,0},{0,0}};
    int  score = 0;

    if (ai_choose_move(&args->b_snapshot, &args->gs_snapshot,
                       args->me_snapshot, &best, &score))
    {
        AiMoveCtx *ctx = g_new(AiMoveCtx, 1);
        ctx->app = app;
        ctx->m   = best;
        g_idle_add((GSourceFunc)apply_ai_move_cb, ctx);
    } else {
        app->ai_busy = 0;
    }

    app->ai_thread_live = 0;
    g_free(args);
    return NULL;
}

/**
 * \brief Callback qui applique réellement le coup choisi par l’IA.
 * \details
 * Appelé après un court délai pour laisser voir la case “from”
 * en surbrillance (effet de sélection).
 */
static gboolean apply_ai_move_cb(gpointer data){
    AiMoveCtx* c = (AiMoveCtx*)data;
    App* app = c->app;
    Move m = c->m;

    char s[6]; move_to_str(m, s);
    if(my_player(app)==PLAYER_BLUE){
        printf("Je (bleu) joue : %s\n", s);
        printf("Je (bleu) envoie : %s\n", s);
    }else{
        printf("Je (rouge) joue : %s\n", s);
        printf("Je (rouge) envoie : %s\n", s);
    }

    app->sel = m.from;
    app->has_sel = true;
    gtk_widget_queue_draw(app->area);

    GameResult gr = GAME_ONGOING;
    if (apply_move(&app->b, &app->gs, m, &gr)){
        app->last_result = gr;
        app->has_sel = false;
        update_sidebar(app);
        gtk_widget_queue_draw(app->area);
        send_move_if_net(app, m);
        if(gr!=GAME_ONGOING) show_victory_dialog(app);
    }
    app->ai_busy = 0;
    g_free(c);

    if (is_local_ai_turn(app)) {
        schedule_ai_if_needed(app);
    }
    return G_SOURCE_REMOVE;
}

/**
 * \brief Choisit un coup IA après un délai “réflexion”, montre la sélection,
 * puis applique réellement le coup un peu plus tard.
 */
static gboolean ai_play_after_delay(gpointer udata){
    App* app = (App*)udata;
    app->ai_timer_id = 0;
    if(!is_local_ai_turn(app)){ app->ai_busy=0; return G_SOURCE_REMOVE; }
    if(app->ai_busy){ return G_SOURCE_REMOVE; }
    if(app->ai_thread_live){ return G_SOURCE_REMOVE; }

    app->ai_busy = 1;
    app->ai_thread_live = 1;

    AiWorkerArgs *args = g_new0(AiWorkerArgs, 1);
    args->b_snapshot  = app->b;
    args->gs_snapshot = app->gs;
    args->me_snapshot = my_player(app);
    args->app         = app;

    app->ai_thread = g_thread_new("ai_worker", ai_worker_func, args);
    return G_SOURCE_REMOVE;
}

/**
 * \brief Planifie l’IA avec un petit délai “humain” (ne planifie qu’une fois).
 * \details
 * Si un timer est déjà en place, on ne replannifie pas.
 */
static void schedule_ai_if_needed(App* app){
    if(!is_local_ai_turn(app)) return;
    if(app->ai_timer_id) return;  /* déjà prévu */
    int d = app->ai_delay_ms>0 ? app->ai_delay_ms : 700;
    app->ai_timer_id = g_timeout_add(d, ai_play_after_delay, app);
}

/* ============================================================
 * ======================   PARTIE RÉSEAU   ===================
 * ============================================================
 * Idée : un thread écoute les 4 octets reçus. L’UI est mise
 * à jour via g_idle_add(). On gère aussi un message RSET.
 */

/** \brief Petit paquet pour appliquer un coup reçu (thread->UI). */
typedef struct { App* app; char four[4]; } RecvMoveData;

/**
 * \brief Applique un message reçu (coup ou RSET) côté UI.
 * \details
 * - “RSET” : on relance la partie des deux côtés, Bleu (client) commence.
 * - Coup “A1A3” : on le valide puis on redonne éventuellement la main à l’IA locale.
 */
static gboolean apply_received_move_cb(gpointer udata){
    RecvMoveData* d = (RecvMoveData*)udata;
    App* app = d->app;

    /* Gestion RSET (redémarrage synchro) */
    if(d->four[0]=='R' && d->four[1]=='S' && d->four[2]=='E' && d->four[3]=='T'){
        app->b = app->b0;
        app->gs = app->gs0;
        app->last_result = check_victory(&app->b, &app->gs);
        app->has_sel = false;
        if(app->ai_timer_id){ g_source_remove(app->ai_timer_id); app->ai_timer_id=0; }
        if(app->net_state){
            if(app->net_state->mode==GAME_MODE_CLIENT) app->net_state->is_my_turn = TRUE;  /* client = bleu commence */
            else if(app->net_state->mode==GAME_MODE_SERVER) app->net_state->is_my_turn = FALSE;
        }
        update_sidebar(app);
        gtk_widget_queue_draw(app->area);
        post_label_update(app->lbl_net, "Nouvelle partie", 1);
        schedule_ai_if_needed(app);
        g_free(d);
        return G_SOURCE_REMOVE;
    }

    /* Coup normal */
    Move m;
    if(!move_from_4chars(d->four, &m)){ g_free(d); return G_SOURCE_REMOVE; }

    Player opp = (app->net_state && app->net_state->mode==GAME_MODE_SERVER)? PLAYER_BLUE : PLAYER_RED;
    char s[6]; move_to_str(m, s);
    if(opp==PLAYER_BLUE) printf("L'adversaire (bleu) propose : %s\n", s);
    else                 printf("L'adversaire (rouge) propose : %s\n", s);

    GameResult gr = GAME_ONGOING;
    if (!apply_move(&app->b, &app->gs, m, &gr)){
        printf("La proposition %s est illégale - arrêt\n",
               (opp==PLAYER_BLUE?"bleu":"rouge"));
        g_free(d);
        return G_SOURCE_REMOVE;
    }
    if(opp==PLAYER_BLUE) puts("La proposition bleu est validée");
    else                 puts("La proposition rouge est validée");

    app->last_result = gr;
    app->has_sel = false;
    if(app->net_state) app->net_state->is_my_turn = true;
    update_sidebar(app);
    gtk_widget_queue_draw(app->area);
    if(gr!=GAME_ONGOING) show_victory_dialog(app);

    g_free(d);
    /* Peut-être que c’est à l’IA locale de jouer maintenant */
    schedule_ai_if_needed(app);
    return G_SOURCE_REMOVE;
}

/**
 * \brief Thread réseau principal :
 * - serveur : écoute puis accepte ; client : se connecte ;
 * - ensuite, boucle de réception de messages 4 octets.
 */
static void* network_thread_func(void* arg){
    App* app = (App*)arg; NetworkState* net = app->net_state;
    if(!net){ app->network_thread_running=false; return NULL; }

    if(net->mode==GAME_MODE_SERVER){
        int lfd = net_server_listen(net->port);
        if(lfd<0){ app->network_thread_running=false; return NULL; }
        post_label_update(app->lbl_net, "Serveur : en attente de co…", 1);
        int cfd = net_server_accept(lfd);
        net_close(lfd);
        if(cfd<0){ app->network_thread_running=false; return NULL; }
        net->net_fd=cfd; net->connected=true;
        post_label_update(app->lbl_net, "Serveur : client connecté.", 1);
    } else if(net->mode==GAME_MODE_CLIENT){
        const char* host = (net->host[0]? net->host : "127.0.0.1");
        uint16_t port = (net->port? net->port : 8080);
        post_label_update(app->lbl_net, "Client : connexion…", 1);
        int fd = net_client_connect(host, port);
        if(fd<0){ app->network_thread_running=false; return NULL; }
        net->net_fd=fd; net->connected=true;
        post_label_update(app->lbl_net, "Client : connecté au serveur.", 1);
    } else {
        post_label_update(app->lbl_net, "", 0);
    }

    if(net->connected && net->net_fd>=0){
        for(;;){
            char four[4];
            int rr = net_recv4(net->net_fd, four);
            if(rr<=0){
                post_label_update(app->lbl_net, "Réseau : connexion fermée.", 1);
                break;
            }
            if(four[0]=='Q'&&four[1]=='U'&&four[2]=='I'&&four[3]=='T'){
                post_label_update(app->lbl_net, "Réseau : fin de partie (pair).", 1);
                break;
            }
            RecvMoveData* d = g_new(RecvMoveData,1);
            d->app = app; memcpy(d->four,four,4);
            g_idle_add(apply_received_move_cb, d);
        }
    }
    app->network_thread_running=false;
    return NULL;
}

/* ============================================================
 * ================   INTERACTIONS (LOCAL)   ==================
 * ============================================================
 * Clics souris : sélection puis déplacement si possible.
 * Échap : dé-sélection.
 */

/**
 * \brief Gère le clic sur le plateau (mode local).
 * \details
 * - 1er clic : sélectionne une pièce alliée ;
 * - 2e clic : si valide => applique le coup ; sinon => met juste à jour la sélection.
 */
static gboolean on_click(GtkGestureClick *gesture, gint n_press, gdouble x, gdouble y, gpointer user_data){
    (void)gesture; (void)n_press;
    App* app = (App*)user_data;
    if (app->last_result != GAME_ONGOING) return TRUE;

    int W = gtk_widget_get_width(app->area);
    int H = gtk_widget_get_height(app->area);
    const int N = BOARD_SIZE;
    const double M = 20.0;
    double maxW = (W - 2*M), maxH = (H - 2*M);
    if(maxW<100) maxW=100;
    if(maxH<100) maxH=100;
    double cell = ((maxW < maxH ? maxW : maxH) / (double)N);
    double offx = (W - cell*N)/2.0;
    double offy = (H - cell*N)/2.0;

    int gx = (int)((x - offx) / cell);
    int gy = (int)((y - offy) / cell);
    if (gx < 0 || gx >= N || gy < 0 || gy >= N) return TRUE;

    Coord clicked = (Coord){gx, gy};

    if (!app->has_sel){
        Piece p = board_get(&app->b, clicked);
        if (p != EMPTY && piece_is_ally(p, app->b.current_player)){
            app->sel = clicked;
            app->has_sel = true;
            update_sidebar(app);
            gtk_widget_queue_draw(app->area);
        }
        return TRUE;
    } else {
        Piece p = board_get(&app->b, clicked);
        if (p != EMPTY && piece_is_ally(p, app->b.current_player)){
            app->sel = clicked;
            app->has_sel = true;
            update_sidebar(app);
            gtk_widget_queue_draw(app->area);
            return TRUE;
        }
        if (clicked.x == app->sel.x && clicked.y == app->sel.y){
            app->has_sel = false;
            update_sidebar(app);
            gtk_widget_queue_draw(app->area);
            return TRUE;
        }
        Move m = (Move){app->sel, clicked};
        GameResult gr = GAME_ONGOING;
        if (apply_move(&app->b, &app->gs, m, &gr)){
            app->last_result = gr;
            app->has_sel = false;
            update_sidebar(app);
            gtk_widget_queue_draw(app->area);
            show_victory_dialog(app);
        } else {
            update_sidebar(app);
            gtk_widget_queue_draw(app->area);
        }
        return TRUE;
    }
}

/**
 * \brief Gère le clic sur le plateau (mode réseau).
 * \details
 * - Si l’IA locale joue ce côté, on bloque les clics quand ce n’est pas notre tour.
 * - Sinon, même logique que local + envoi du coup sur le réseau.
 */
static gboolean on_click_network(GtkGestureClick *gesture, gint n_press, gdouble x, gdouble y, gpointer user_data){
    (void)gesture; (void)n_press;
    App* app = (App*)user_data;

    /* Si IA locale active : on ignore les clics pendant son tour */
    if (app->ai_enabled){
        if (app->net_state && !app->net_state->is_my_turn) return TRUE;
        if (app->b.current_player != my_player(app)) return TRUE;
    }

    if (app->last_result != GAME_ONGOING) return TRUE;

    if (app->net_state && app->net_state->mode!=GAME_MODE_LOCAL && !app->net_state->is_my_turn)
        return TRUE;

    int W = gtk_widget_get_width(app->area);
    int H = gtk_widget_get_height(app->area);
    const int N = BOARD_SIZE;
    const double M = 20.0;
    double maxW = (W - 2*M), maxH = (H - 2*M);
    if(maxW<100) maxW=100;
    if(maxH<100) maxH=100;
    double cell = ((maxW < maxH ? maxW : maxH) / (double)N);
    double offx = (W - cell*N)/2.0;
    double offy = (H - cell*N)/2.0;

    int gx = (int)((x - offx) / cell);
    int gy = (int)((y - offy) / cell);
    if (gx < 0 || gx >= N || gy < 0 || gy >= N) return TRUE;

    Coord clicked = (Coord){gx, gy};

    if (!app->has_sel){
        Piece p = board_get(&app->b, clicked);
        if (p != EMPTY && piece_is_ally(p, app->b.current_player)){
            app->sel = clicked;
            app->has_sel = true;
            update_sidebar(app);
            gtk_widget_queue_draw(app->area);
        }
        return TRUE;
    } else {
        Piece p = board_get(&app->b, clicked);
        if (p != EMPTY && piece_is_ally(p, app->b.current_player)){
            app->sel = clicked;
            app->has_sel = true;
            update_sidebar(app);
            gtk_widget_queue_draw(app->area);
            return TRUE;
        }
        if (clicked.x == app->sel.x && clicked.y == app->sel.y){
            app->has_sel = false;
            update_sidebar(app);
            gtk_widget_queue_draw(app->area);
            return TRUE;
        }
        Move m = (Move){app->sel, clicked};
        GameResult gr = GAME_ONGOING;
        if (apply_move(&app->b, &app->gs, m, &gr)){
            app->last_result = gr;
            app->has_sel = false;
            update_sidebar(app);
            gtk_widget_queue_draw(app->area);
            if(gr!=GAME_ONGOING) show_victory_dialog(app);

            if (app->net_state && app->net_state->mode!=GAME_MODE_LOCAL && app->net_state->connected){
                char four[4]; move_to_4chars(m, four);
                net_send4(app->net_state->net_fd, four);
                app->net_state->is_my_turn = false;
            }
            /* Après un coup local : peut-être à l’IA ensuite */
            schedule_ai_if_needed(app);
        } else {
            update_sidebar(app);
            gtk_widget_queue_draw(app->area);
        }
        return TRUE;
    }
}

/** \brief Échap = dé-sélectionner. */
static gboolean on_key(GtkEventControllerKey* ctrl, guint keyval, guint keycode, GdkModifierType state, gpointer user_data){
    (void)ctrl; (void)keycode; (void)state;
    App* app = (App*)user_data;
    if(keyval == GDK_KEY_Escape && app->has_sel){
        app->has_sel = false;
        update_sidebar(app);
        gtk_widget_queue_draw(app->area);
        return TRUE;
    }
    return FALSE;
}

/**
 * \brief Bouton “Recommencer” : remet le jeu à l’état initial et resynchronise.
 * \details
 * - Annule un timer IA en cours.
 * - En réseau : envoie “RSET”, client (bleu) recommence.
 */
static void on_restart(GtkButton* btn, gpointer user_data){
    (void)btn;
    App* app = (App*)user_data;

    /* Reset local */
    app->b = app->b0;
    app->gs = app->gs0;
    app->last_result = check_victory(&app->b, &app->gs);
    app->has_sel = false;

    /* Annule timer IA en cours */
    if(app->ai_timer_id){ g_source_remove(app->ai_timer_id); app->ai_timer_id=0; }
    app->ai_busy = 0;

    /* Join du thread IA si encore actif */
    if(app->ai_thread_live && app->ai_thread){
        g_thread_join(app->ai_thread);
        app->ai_thread = NULL;
        app->ai_thread_live = 0;
    }

    /* Si réseau : envoie RSET, client (BLEU) commence */
    if(app->net_state && app->net_state->mode!=GAME_MODE_LOCAL && app->net_state->connected){
        static const char rset4[4]={'R','S','E','T'};
        net_send4(app->net_state->net_fd, rset4);
        if(app->net_state->mode==GAME_MODE_CLIENT) app->net_state->is_my_turn = TRUE;
        else if(app->net_state->mode==GAME_MODE_SERVER) app->net_state->is_my_turn = FALSE;
    }

    update_sidebar(app);
    gtk_widget_queue_draw(app->area);

    /* Relance IA si elle doit commencer ici */
    schedule_ai_if_needed(app);
}

/** \brief Nettoyage final quand la fenêtre est détruite. */
static void on_destroy(GtkWindow* w, gpointer user_data){
    (void)w;
    App* app=(App*)user_data;
    if(app->ai_timer_id){ g_source_remove(app->ai_timer_id); app->ai_timer_id=0; }

    /* Join du thread IA si en cours */
    if(app->ai_thread_live && app->ai_thread){
        g_thread_join(app->ai_thread);
        app->ai_thread = NULL;
        app->ai_thread_live = 0;
    }

    free_assets(app);
    g_free(app);
}

/* ============================================================
 * =====================   LANCEMENT (LOCAL)   ================
 * ============================================================
 */

/**
 * \brief Lance l’UI en mode local (sans réseau, IA désactivée ici).
 * \param app application GTK
 * \param initial_board plateau initial
 * \param initial_state état de jeu initial
 */
void ui_run_gui(GtkApplication* app, Board* initial_board, GameState* initial_state){
    App* st = g_new0(App, 1);
    st->b  = *initial_board;
    st->gs = *initial_state;
    st->b0  = *initial_board;
    st->gs0 = *initial_state;
    st->last_result = check_victory(&st->b, &st->gs);
    st->has_sel = false;
    st->net_state = NULL;
    st->ai_enabled = 0;
    st->ai_busy = 0;
    st->ai_delay_ms = 700;
    st->ai_timer_id = 0;
    st->ai_thread = NULL; st->ai_thread_live = 0;

    load_assets(st);

    st->win = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(st->win), "Krojanty");
    gtk_window_set_default_size(GTK_WINDOW(st->win), 800, 500);
    gtk_window_set_resizable(GTK_WINDOW(st->win), FALSE);
    g_signal_connect(st->win, "destroy", G_CALLBACK(on_destroy), st);

    load_sidebar_css();

    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_window_set_child(GTK_WINDOW(st->win), hbox);

    st->sidebar = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_add_css_class(st->sidebar, "sidebar");
    gtk_widget_set_size_request(st->sidebar, 200, -1);
    gtk_box_append(GTK_BOX(hbox), st->sidebar);

    st->lbl_game_title = gtk_label_new("KROJANTY");
    gtk_widget_add_css_class(st->lbl_game_title, "game-title");
    gtk_box_append(GTK_BOX(st->sidebar), st->lbl_game_title);

    st->lbl_net = gtk_label_new("");
    gtk_widget_add_css_class(st->lbl_net, "net-badge");
    gtk_widget_set_visible(st->lbl_net, FALSE);
    gtk_box_append(GTK_BOX(st->sidebar), st->lbl_net);

    GtkWidget* sep = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_add_css_class(sep, "thin-sep");
    gtk_box_append(GTK_BOX(st->sidebar), sep);

    st->lbl_title  = gtk_label_new("");
    st->lbl_turn   = gtk_label_new("");
    st->lbl_player = gtk_label_new("");
    st->lbl_pts_b  = gtk_label_new("");
    st->lbl_pts_r  = gtk_label_new("");
    st->lbl_cap_b  = gtk_label_new("");
    st->lbl_cap_r  = gtk_label_new("");
    st->lbl_kings  = gtk_label_new("");
    st->lbl_sel    = gtk_label_new("");

    gtk_widget_add_css_class(st->lbl_title,  "score-title");
    gtk_widget_add_css_class(st->lbl_turn,   "score-item"); gtk_widget_add_css_class(st->lbl_turn,   "spaced");
    gtk_widget_add_css_class(st->lbl_player, "score-item"); gtk_widget_add_css_class(st->lbl_player, "spaced");
    gtk_widget_add_css_class(st->lbl_pts_b,  "score-item"); gtk_widget_add_css_class(st->lbl_pts_b,  "spaced");
    gtk_widget_add_css_class(st->lbl_pts_r,  "score-item"); gtk_widget_add_css_class(st->lbl_pts_r,  "spaced");
    gtk_widget_add_css_class(st->lbl_cap_b,  "score-item"); gtk_widget_add_css_class(st->lbl_cap_b,  "spaced");
    gtk_widget_add_css_class(st->lbl_cap_r,  "score-item"); gtk_widget_add_css_class(st->lbl_cap_r,  "spaced");
    gtk_widget_add_css_class(st->lbl_kings,  "score-item"); gtk_widget_add_css_class(st->lbl_kings,  "spaced");
    gtk_widget_add_css_class(st->lbl_sel,    "score-item"); gtk_widget_add_css_class(st->lbl_sel,    "spaced");

    gtk_box_append(GTK_BOX(st->sidebar), st->lbl_title);
    gtk_box_append(GTK_BOX(st->sidebar), st->lbl_turn);
    gtk_box_append(GTK_BOX(st->sidebar), st->lbl_player);

    /* --- Encadré BLEU --- */
    GtkWidget* blue_title = gtk_label_new("ÉQUIPE BLEUE");
    gtk_widget_add_css_class(blue_title, "team-title");
    gtk_box_append(GTK_BOX(st->sidebar), blue_title);

    GtkWidget* blue_panel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_widget_add_css_class(blue_panel, "team-panel");
    gtk_widget_add_css_class(blue_panel, "team-blue");
    gtk_box_append(GTK_BOX(st->sidebar), blue_panel);

    gtk_box_append(GTK_BOX(blue_panel), st->lbl_pts_b);
    gtk_box_append(GTK_BOX(blue_panel), st->lbl_cap_b);

    /* --- Encadré ROUGE --- */
    GtkWidget* red_title = gtk_label_new("ÉQUIPE ROUGE");
    gtk_widget_add_css_class(red_title, "team-title");
    gtk_box_append(GTK_BOX(st->sidebar), red_title);

    GtkWidget* red_panel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_widget_add_css_class(red_panel, "team-panel");
    gtk_widget_add_css_class(red_panel, "team-red");
    gtk_box_append(GTK_BOX(st->sidebar), red_panel);

    gtk_box_append(GTK_BOX(red_panel), st->lbl_pts_r);
    gtk_box_append(GTK_BOX(red_panel), st->lbl_cap_r);

    /* --- Infos communes --- */
    gtk_box_append(GTK_BOX(st->sidebar), st->lbl_kings);
    gtk_box_append(GTK_BOX(st->sidebar), st->lbl_sel);


    st->btn_restart = gtk_button_new_with_label("↻ Recommencer");
    gtk_widget_add_css_class(st->btn_restart, "big");
    gtk_box_append(GTK_BOX(st->sidebar), st->btn_restart);
    g_signal_connect(st->btn_restart, "clicked", G_CALLBACK(on_restart), st);

    GtkWidget *board_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_hexpand(board_box, TRUE);
    gtk_widget_set_vexpand(board_box, TRUE);
    gtk_box_append(GTK_BOX(hbox), board_box);

    st->area = gtk_drawing_area_new();
    gtk_widget_set_hexpand(st->area, TRUE);
    gtk_widget_set_vexpand(st->area, TRUE);
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(st->area), draw_board, st, NULL);
    gtk_box_append(GTK_BOX(board_box), st->area);

    GtkGesture *click = gtk_gesture_click_new();
    g_signal_connect(click, "released", G_CALLBACK(on_click), st);
    gtk_widget_add_controller(st->area, GTK_EVENT_CONTROLLER(click));

    GtkEventController* key = gtk_event_controller_key_new();
    g_signal_connect(key, "key-pressed", G_CALLBACK(on_key), st);
    gtk_widget_add_controller(st->win, key);

    update_sidebar(st);
    gtk_window_present(GTK_WINDOW(st->win));
}

/* ============================================================
 * =====================   LANCEMENT (RÉSEAU)   ===============
 * ============================================================
 */

/**
 * \brief Lance l’UI réseau. L’IA locale peut être activée côté client/serveur.
 * \param app application GTK
 * \param initial_board plateau initial
 * \param initial_state état initial
 * \param net_state configuration réseau (mode client/serveur)
 * \param ai_enabled 1 si on veut que ce côté soit joué par l’IA
 * \param ai_depth profondeur de recherche de l’IA
 */
void ui_run_gui_network(GtkApplication* app,
                        Board* initial_board,
                        GameState* initial_state,
                        NetworkState* net_state,
                        int ai_enabled,
                        int ai_depth){
    App* st = g_new0(App, 1);
    st->b  = *initial_board;
    st->gs = *initial_state;
    st->b0  = *initial_board;
    st->gs0 = *initial_state;
    st->last_result = check_victory(&st->b, &st->gs);
    st->has_sel = false;
    st->net_state = net_state;
    st->network_thread_running = false;

    st->ai_enabled = ai_enabled? 1:0;
    st->ai_busy = 0;
    st->gs.Depth = (ai_depth>0? ai_depth:3);
    st->gs.UseAI = 1;
    st->gs0.Depth = (ai_depth>0? ai_depth:3);
    st->gs0.UseAI = 1;
    st->ai_delay_ms = 700;
    st->ai_timer_id = 0;
    st->ai_thread = NULL; st->ai_thread_live = 0;

    load_assets(st);

    st->win = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(st->win), "Krojanty");
    gtk_window_set_default_size(GTK_WINDOW(st->win), 800, 500);
    gtk_window_set_resizable(GTK_WINDOW(st->win), FALSE);
    g_signal_connect(st->win, "destroy", G_CALLBACK(on_destroy), st);

    load_sidebar_css();

    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_window_set_child(GTK_WINDOW(st->win), hbox);

    st->sidebar = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_add_css_class(st->sidebar, "sidebar");
    gtk_widget_set_size_request(st->sidebar, 200, -1);
    gtk_box_append(GTK_BOX(hbox), st->sidebar);

    st->lbl_game_title = gtk_label_new("KROJANTY");
    gtk_widget_add_css_class(st->lbl_game_title, "game-title");
    gtk_box_append(GTK_BOX(st->sidebar), st->lbl_game_title);

    st->lbl_net = gtk_label_new("");
    gtk_widget_add_css_class(st->lbl_net, "net-badge");
    gtk_widget_set_visible(st->lbl_net, TRUE);
    if (net_state->mode == GAME_MODE_SERVER)
        gtk_label_set_text(GTK_LABEL(st->lbl_net), "Serveur : en attente de connexion…");
    else if (net_state->mode == GAME_MODE_CLIENT)
        gtk_label_set_text(GTK_LABEL(st->lbl_net), "Client : connexion…");
    else
        gtk_widget_set_visible(st->lbl_net, FALSE);
    gtk_box_append(GTK_BOX(st->sidebar), st->lbl_net);

    GtkWidget* sep = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_add_css_class(sep, "thin-sep");
    gtk_box_append(GTK_BOX(st->sidebar), sep);

    st->lbl_title  = gtk_label_new("");
    st->lbl_turn   = gtk_label_new("");
    st->lbl_player = gtk_label_new("");
    st->lbl_pts_b  = gtk_label_new("");
    st->lbl_pts_r  = gtk_label_new("");
    st->lbl_cap_b  = gtk_label_new("");
    st->lbl_cap_r  = gtk_label_new("");
    st->lbl_kings  = gtk_label_new("");
    st->lbl_sel    = gtk_label_new("");

    gtk_widget_add_css_class(st->lbl_title,  "score-title");
    gtk_widget_add_css_class(st->lbl_turn,   "score-item"); gtk_widget_add_css_class(st->lbl_turn,   "spaced");
    gtk_widget_add_css_class(st->lbl_player, "score-item"); gtk_widget_add_css_class(st->lbl_player, "spaced");
    gtk_widget_add_css_class(st->lbl_pts_b,  "score-item"); gtk_widget_add_css_class(st->lbl_pts_b,  "spaced");
    gtk_widget_add_css_class(st->lbl_pts_r,  "score-item"); gtk_widget_add_css_class(st->lbl_pts_r,  "spaced");
    gtk_widget_add_css_class(st->lbl_cap_b,  "score-item"); gtk_widget_add_css_class(st->lbl_cap_b,  "spaced");
    gtk_widget_add_css_class(st->lbl_cap_r,  "score-item"); gtk_widget_add_css_class(st->lbl_cap_r,  "spaced");
    gtk_widget_add_css_class(st->lbl_kings,  "score-item"); gtk_widget_add_css_class(st->lbl_kings,  "spaced");
    gtk_widget_add_css_class(st->lbl_sel,    "score-item"); gtk_widget_add_css_class(st->lbl_sel,    "spaced");

    gtk_box_append(GTK_BOX(st->sidebar), st->lbl_title);
    gtk_box_append(GTK_BOX(st->sidebar), st->lbl_turn);
    gtk_box_append(GTK_BOX(st->sidebar), st->lbl_player);

    /* --- Encadré BLEU --- */
    GtkWidget* blue_title = gtk_label_new("ÉQUIPE BLEUE");
    gtk_widget_add_css_class(blue_title, "team-title");
    gtk_box_append(GTK_BOX(st->sidebar), blue_title);

    GtkWidget* blue_panel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_widget_add_css_class(blue_panel, "team-panel");
    gtk_widget_add_css_class(blue_panel, "team-blue");
    gtk_box_append(GTK_BOX(st->sidebar), blue_panel);

    gtk_box_append(GTK_BOX(blue_panel), st->lbl_pts_b);
    gtk_box_append(GTK_BOX(blue_panel), st->lbl_cap_b);

    /* --- Encadré ROUGE --- */
    GtkWidget* red_title = gtk_label_new("ÉQUIPE ROUGE");
    gtk_widget_add_css_class(red_title, "team-title");
    gtk_box_append(GTK_BOX(st->sidebar), red_title);

    GtkWidget* red_panel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_widget_add_css_class(red_panel, "team-panel");
    gtk_widget_add_css_class(red_panel, "team-red");
    gtk_box_append(GTK_BOX(st->sidebar), red_panel);

    gtk_box_append(GTK_BOX(red_panel), st->lbl_pts_r);
    gtk_box_append(GTK_BOX(red_panel), st->lbl_cap_r);

    /* --- Infos communes --- */
    gtk_box_append(GTK_BOX(st->sidebar), st->lbl_kings);
    gtk_box_append(GTK_BOX(st->sidebar), st->lbl_sel);

    const char* btn_text = (net_state->mode==GAME_MODE_SERVER)
        ? "↻ J'ai pas dit mon dernier mot"
        : "↻ J'ai pas dit mon dernier mot";
    st->btn_restart = gtk_button_new_with_label(btn_text);
    gtk_widget_add_css_class(st->btn_restart, "big");
    gtk_box_append(GTK_BOX(st->sidebar), st->btn_restart);
    g_signal_connect(st->btn_restart, "clicked", G_CALLBACK(on_restart), st);

    GtkWidget *board_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_hexpand(board_box, TRUE);
    gtk_widget_set_vexpand(board_box, TRUE);
    gtk_box_append(GTK_BOX(hbox), board_box);

    st->area = gtk_drawing_area_new();
    gtk_widget_set_hexpand(st->area, TRUE);
    gtk_widget_set_vexpand(st->area, TRUE);
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(st->area), draw_board, st, NULL);
    gtk_box_append(GTK_BOX(board_box), st->area);

    GtkGesture *click = gtk_gesture_click_new();
    g_signal_connect(click, "released", G_CALLBACK(on_click_network), st);
    gtk_widget_add_controller(st->area, GTK_EVENT_CONTROLLER(click));

    GtkEventController* key = gtk_event_controller_key_new();
    g_signal_connect(key, "key-pressed", G_CALLBACK(on_key), st);
    gtk_widget_add_controller(st->win, key);

    /* Thread réseau si nécessaire */
    if (net_state->mode == GAME_MODE_SERVER || net_state->mode == GAME_MODE_CLIENT){
        st->network_thread_running = true;
        pthread_create(&st->network_thread, NULL, network_thread_func, st);
    }

    update_sidebar(st);
    gtk_window_present(GTK_WINDOW(st->win));

    /* Si c'est à nous de jouer dès le départ et IA activée (client=bleu commence) */
    schedule_ai_if_needed(st);
}